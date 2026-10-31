#include "PalmeraJoueur.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PalmeraConfig.h"
#include "PalmeraCourse.h"
#include "Camera/CameraActor.h"
#include "PalmeraHeros.h"
#include "PalmeraHistoire.h"
#include "PalmeraHud.h"
#include "PalmeraPNJ.h"
#include "PalmeraPartie.h"
#include "PalmeraSons.h"
#include "PalmeraVoiture.h"
#include "Widgets/Layout/SDPIScaler.h"

APalmeraJoueur::APalmeraJoueur()
{
	bShowMouseCursor = false;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void APalmeraJoueur::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	bPiloteTest = FParse::Param(FCommandLine::Get(), TEXT("pilote"));
	const bool bSansAide = bPiloteTest || FParse::Param(FCommandLine::Get(), TEXT("sansaide"));

	if (GEngine && GEngine->GameViewport)
	{
		// L'interface garde la même taille apparente quelle que soit la hauteur de l'écran
		const auto Echelle = TAttribute<float>::CreateLambda([]()
		{
			FVector2D Taille(1920.0, 1080.0);
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(Taille);
			}
			return float(FMath::Max(Taille.Y, 600.0) / 1080.0);
		});
		SAssignNew(Hud, SPalmeraHud).Joueur(this);
		Hud->bAideOuverte = !bSansAide;
		EnveloppeHud = SNew(SDPIScaler).DPIScale(Echelle)[Hud.ToSharedRef()];
		GEngine->GameViewport->AddViewportWidgetContent(EnveloppeHud.ToSharedRef(), 10);
	}
	int32 Niveau = 2;
	GConfig->GetInt(TEXT("Palmera"), TEXT("Qualite"), Niveau, GGameUserSettingsIni);
	AppliquerQualite(FParse::Param(FCommandLine::Get(), TEXT("qualitetest")) ? 2 : Niveau);
	bParcours = FParse::Param(FCommandLine::Get(), TEXT("parcours"));
	bEssaiVirage = FParse::Param(FCommandLine::Get(), TEXT("essaivirage"));
	bReleve = FParse::Param(FCommandLine::Get(), TEXT("releve"));
}

void APalmeraJoueur::EndPlay(const EEndPlayReason::Type Raison)
{
	if (EnveloppeHud.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(EnveloppeHud.ToSharedRef());
	}
	EnveloppeHud.Reset();
	Hud.Reset();
	Super::EndPlay(Raison);
}

void APalmeraJoueur::Demarrer(APalmeraHeros* NouveauHeros)
{
	Heros = NouveauHeros;
	Possess(Heros);
}

void APalmeraJoueur::Ramener(const FVector& Position)
{
	if (Voiture)
	{
		Descendre();
	}
	Heros->SetActorLocation(Position, false, nullptr, ETeleportType::ResetPhysics);
	Heros->GetCharacterMovement()->StopMovementImmediately();
}

void APalmeraJoueur::CommencerScene(APalmeraPNJ* Avec, ACameraActor* Camera)
{
	if (Voiture)
	{
		Descendre();
	}
	bEnScene = true;
	if (Heros && Avec)
	{
		const FVector Centre = Avec->GetActorLocation();
		FVector Direction = (Heros->GetActorLocation() - Centre).GetSafeNormal2D();
		if (Direction.IsNearlyZero())
		{
			Direction = Avec->GetActorForwardVector();
		}
		const FVector Place = Centre + Direction * 165.0;
		Heros->SetActorLocationAndRotation(FVector(Place.X, Place.Y, Centre.Z + 3.0), (-Direction).Rotation(), false, nullptr, ETeleportType::ResetPhysics);
		Heros->GetCharacterMovement()->StopMovementImmediately();
		Avec->Regarder(Heros->GetActorLocation());
	}
	SetViewTargetWithBlend(Camera, 0.35f);
}

void APalmeraJoueur::FinirScene()
{
	bEnScene = false;
	SetViewTargetWithBlend(GetPawn(), 0.35f);
}

bool APalmeraJoueur::Touche(FKey A, FKey B, FKey C) const{
	return IsInputKeyDown(A) || (B.IsValid() && IsInputKeyDown(B)) || (C.IsValid() && IsInputKeyDown(C));
}

bool APalmeraJoueur::Appui(FKey A, FKey B) const
{
	return WasInputKeyJustPressed(A) || (B.IsValid() && WasInputKeyJustPressed(B));
}

void APalmeraJoueur::BasculerMenu()
{
	if (!Hud.IsValid())
	{
		return;
	}
	Hud->bMenuOuvert = !Hud->bMenuOuvert;
	Hud->bJournalOuvert = false;
	Hud->ChoixMenu = 0;
	SetPause(Hud->bMenuOuvert);
}

void APalmeraJoueur::AppliquerQualite(int32 Niveau)
{
	if (Hud.IsValid())
	{
		Hud->Qualite = Niveau;
	}
	if (UGameUserSettings* Reglages = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Reglages->SetOverallScalabilityLevel(FMath::Clamp(Niveau, 0, 3));
		Reglages->ApplySettings(false);
	}
	GConfig->SetInt(TEXT("Palmera"), TEXT("Qualite"), Niveau, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	// Sur l'écran très large, l'image est calculée plus petite puis reconstruite par TSR
	static const float Echelles[] = { 50.f, 58.f, 66.f, 75.f };
	if (IConsoleVariable* Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
	{
		Cv->Set(Echelles[FMath::Clamp(Niveau, 0, 3)], ECVF_SetByCode);
	}
	if (IConsoleVariable* Rt = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing")))
	{
		Rt->Set(Niveau >= 1 ? 1 : 0, ECVF_SetByCode);
	}
}

void APalmeraJoueur::CommandesMenu()
{
	const bool bCourse = APalmeraCourse::Lire(this) != nullptr;
	const int32 NombreChoix = bCourse ? 3 : SPalmeraHud::NombreChoix();
	if (Appui(EKeys::Up, EKeys::Gamepad_DPad_Up) || Appui(EKeys::Z, EKeys::W))
	{
		Hud->ChoixMenu = (Hud->ChoixMenu + NombreChoix - 1) % NombreChoix;
	}
	if (Appui(EKeys::Down, EKeys::Gamepad_DPad_Down) || Appui(EKeys::S))
	{
		Hud->ChoixMenu = (Hud->ChoixMenu + 1) % NombreChoix;
	}
	if (Appui(EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom) || Appui(EKeys::SpaceBar))
	{
		switch (bCourse && Hud->ChoixMenu == 2 ? 5 : Hud->ChoixMenu)
		{
		case 0:
			BasculerMenu();
			break;
		case 1:
			AppliquerQualite((Hud->Qualite + 1) % 4);
			break;
		case 2:
			Hud->bJournalOuvert = !Hud->bJournalOuvert;
			break;
		case 3:
			Hud->bCarteOuverte = !Hud->bCarteOuverte;
			break;
		case 4:
			if (APalmeraPartie* Partie = APalmeraPartie::Lire(this))
			{
				Partie->Sauvegarder();
				Partie->Annoncer(TEXT("Partie sauvegardée"), 2.0);
			}
			BasculerMenu();
			break;
		default:
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
			break;
		}
	}
}

void APalmeraJoueur::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bReleve)
	{
		TempsPilote += DeltaTime;
		if (TempsPilote >= ProchainReleve)
		{
			ProchainReleve = TempsPilote + 0.5;
			const APawn* Moi = GetPawn();
			const FVector P = Moi ? Moi->GetActorLocation() / 100.0 : FVector::ZeroVector;
			Noter(FString::Printf(TEXT("pos=(%.1f,%.1f,%.1f) vitesse=%.1f m/s cap=%.0f camera=%.0f voiture=%d menu=%d carte=%d choix=%d qualite=%d pause=%d"),
				P.X, P.Y, P.Z, Moi ? Moi->GetVelocity().Size() / 100.0 : 0.0, Moi ? Moi->GetActorRotation().Yaw : 0.0, GetControlRotation().Yaw, Voiture ? 1 : 0,
				Hud.IsValid() && Hud->bMenuOuvert ? 1 : 0, Hud.IsValid() && Hud->bCarteOuverte ? 1 : 0, Hud.IsValid() ? Hud->ChoixMenu : 0, Hud.IsValid() ? Hud->Qualite : -1, IsPaused() ? 1 : 0));
		}
	}

	bAction = false;
	if (Hud.IsValid() && (Appui(EKeys::Escape, EKeys::Gamepad_Special_Right) || Appui(EKeys::P)))
	{
		BasculerMenu();
	}
	if (Hud.IsValid() && Hud->bMenuOuvert)
	{
		CommandesMenu();
		if (bParcours)
		{
			PiloteDeTest(DeltaTime);
		}
		return;
	}
	if (Hud.IsValid() && (Appui(EKeys::M, EKeys::Tab) || Appui(EKeys::Gamepad_Special_Left)))
	{
		Hud->bCarteOuverte = !Hud->bCarteOuverte;
	}
	if (Hud.IsValid())
	{
		if (Appui(EKeys::F1))
		{
			Hud->bAideOuverte = !Hud->bAideOuverte;
		}
		else if (Hud->bAideOuverte && Appui(EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom))
		{
			Hud->bAideOuverte = false;
		}
	}
	bAction = Appui(EKeys::E, EKeys::Gamepad_FaceButton_Right);

	// Dialogue : Entrée, Espace ou E font avancer ; les flèches choisissent une réponse
	if (APalmeraHistoire* Recit = APalmeraHistoire::Lire(this))
	{
		if (Recit->bTestPas && Appui(EKeys::F9))
		{
			Recit->SauterEtape(false);
		}
		if (Recit->SceneActive() || Recit->ChoixOuvert())
		{
			bAction = false;
			if (Recit->ChoixOuvert())
			{
				if (Appui(EKeys::Up, EKeys::Gamepad_DPad_Up) || Appui(EKeys::Z, EKeys::W))
				{
					Recit->DeplacerSelection(-1);
				}
				if (Appui(EKeys::Down, EKeys::Gamepad_DPad_Down) || Appui(EKeys::S))
				{
					Recit->DeplacerSelection(1);
				}
			}
			if (Appui(EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom) || Appui(EKeys::SpaceBar, EKeys::E))
			{
				Recit->Avancer();
			}
		}
	}
	if (bEnScene)
	{
		return;
	}

	if (bPiloteTest)
	{
		PiloteDeTest(DeltaTime);
		return;
	}
	if (Voiture)
	{
		CommandesAuVolant(DeltaTime);
	}
	else if (Heros)
	{
		CommandesAPied(DeltaTime);
	}
}

// Écrit une ligne de suivi à côté des photos : le journal n'existe pas dans la version finale du jeu.
void APalmeraJoueur::Noter(const FString& Ligne) const
{
	FString Photo;
	if (FParse::Value(FCommandLine::Get(), TEXT("-photo="), Photo))
	{
		FFileHelper::SaveStringToFile(FString::Printf(TEXT("t=%.1f %s\n"), TempsPilote, *Ligne), *(FPaths::GetPath(Photo) / TEXT("parcours.txt")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	}
}

// Enchaîne tout ce qu'un joueur peut déclencher, sans clavier, et note chaque résultat.
void APalmeraJoueur::ParcoursDeTest(float Dt)
{
	APalmeraPartie* Partie = APalmeraPartie::Lire(this);
	APalmeraSons* Sons = APalmeraSons::Lire(this);
	const auto Etape = [this](double Moment, int32 Numero) { return TempsPilote >= Moment && EtapeParcours == Numero; };
	const auto Etat = [this, Partie]()
	{
		const FVector P = GetPawn() ? GetPawn()->GetActorLocation() / 100.0 : FVector::ZeroVector;
		return FString::Printf(TEXT("argent=%d recherche=%d mission=%d finies=%d cassettes=%d objectif=[%s] message=[%s] pos=(%.0f,%.0f,%.1f) voiture=%d"),
			Partie->Argent, Partie->Recherche, Partie->bEnMission ? 1 : 0, Partie->MissionsFinies, Partie->CassettesTrouvees, *Partie->Objectif, *Partie->Message,
			P.X, P.Y, P.Z, Voiture ? 1 : 0);
	};

	if (Etape(4.0, 0))
	{
		EtapeParcours++;
		Noter(TEXT("depart : ") + Etat());
		Monter();
		Noter(TEXT("monte : ") + Etat());
		if (Sons)
		{
			Sons->StationSuivante();
			Sons->Klaxon();
		}
	}
	else if (Etape(7.0, 1))
	{
		EtapeParcours++;
		Noter(TEXT("radio : ") + (Sons ? Sons->NomStation() : FString(TEXT("pas de sons"))));
		// Arrivée directe au Garage Nina : la mission d'accueil doit se terminer
		Voiture->SetActorLocation(Palmera::EnMetres(-660.0, 124.0, 1.0), false, nullptr, ETeleportType::ResetPhysics);
	}
	else if (Etape(9.0, 2))
	{
		EtapeParcours++;
		Noter(TEXT("mission accueil : ") + Etat());
		if (Partie->Cassettes.Num() > 0)
		{
			const FVector2D C = Partie->Cassettes[0];
			Voiture->SetActorLocation(Palmera::EnMetres(C.X, C.Y, 1.0), false, nullptr, ETeleportType::ResetPhysics);
		}
	}
	else if (Etape(11.0, 3))
	{
		EtapeParcours++;
		Noter(TEXT("cassette : ") + Etat());
		BasculerMenu();
		Noter(FString::Printf(TEXT("menu ouvert, pause=%d"), IsPaused() ? 1 : 0));
		AppliquerQualite(3);
	}
	else if (Etape(13.0, 4))
	{
		EtapeParcours++;
		AppliquerQualite(2);
		BasculerMenu();
		Noter(FString::Printf(TEXT("menu ferme, pause=%d"), IsPaused() ? 1 : 0));
		Hud->bCarteOuverte = true;
	}
	else if (Etape(15.0, 5))
	{
		EtapeParcours++;
		Hud->bCarteOuverte = false;
		Partie->Faute(45.0);
		Noter(TEXT("faute : ") + Etat());
		Partie->Arrete();
		Noter(TEXT("arrete : ") + Etat());
	}
	else if (Etape(18.0, 6))
	{
		EtapeParcours++;
		Noter(TEXT("a pied apres arrestation : ") + Etat());
		// Sur le sable, à 35 m de la promenade : le sol y est à -0,56 m
		Heros->SetActorLocation(Palmera::EnMetres(-1010.0, 120.0, 1.0), false, nullptr, ETeleportType::ResetPhysics);
	}
	else if (Etape(20.0, 7))
	{
		EtapeParcours++;
		Noter(TEXT("a pied sur le sable : ") + Etat());
		Heros->SetActorLocation(Palmera::EnMetres(-990.0, 300.0, 1.5), false, nullptr, ETeleportType::ResetPhysics);
		for (TActorIterator<APalmeraVoiture> It(GetWorld()); It; ++It)
		{
			if (It->PhysiqueActive() && !It->IsHidden())
			{
				It->SetActorLocationAndRotation(Palmera::EnMetres(-990.0, 303.0, 1.2), FRotator(0.0, 180.0, 0.0), false, nullptr, ETeleportType::ResetPhysics);
				break;
			}
		}
		Monter();
		Noter(TEXT("en voiture sur le sable : ") + Etat());
	}
	else if (Voiture && TempsPilote >= 20.5 && TempsPilote < 24.0)
	{
		Voiture->Commander(1.0, 0.0, false);
	}
	else if (Etape(24.0, 8))
	{
		EtapeParcours++;
		Noter(TEXT("apres 3,5 s plein gaz sur le sable : ") + Etat());
		Voiture->Commander(0.0, 0.0, true);
		Descendre();
		Heros->SetActorLocation(Palmera::EnMetres(-1300.0, 0.0, -1.5), false, nullptr, ETeleportType::ResetPhysics);
	}
	else if (Etape(27.0, 9))
	{
		EtapeParcours++;
		Noter(TEXT("repeche : ") + Etat());
		Noter(TEXT("fin du parcours : ") + Etat());
	}
}

// Mesure de tenue de route : à chaque vitesse, volant à fond et pleins gaz pendant 1,2 s sur le front de mer.
void APalmeraJoueur::EssaiVirage(float Dt)
{
	static const double Vitesses[] = { 20.0, 50.0, 90.0, 130.0 };
	static double Debut = 0.0, AngleMax = 0.0, GlisseMax = 0.0, LacetDepart = 0.0;
	static int32 Phase = 0;
	if (!Voiture)
	{
		if (TempsPilote > 3.0 && Heros)
		{
			Monter();
		}
		return;
	}
	if (EtapeParcours >= UE_ARRAY_COUNT(Vitesses))
	{
		Voiture->Commander(0.0, 0.0, true);
		return;
	}
	const double T = TempsPilote;
	if (Phase == 0)
	{
		Voiture->SetActorLocationAndRotation(Palmera::EnMetres(-966.0, 800.0, 1.0), FRotator(0.0, -90.0, 0.0), false, nullptr, ETeleportType::ResetPhysics);
		if (UPrimitiveComponent* Corps = Cast<UPrimitiveComponent>(Voiture->GetRootComponent()))
		{
			Corps->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Corps->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		Voiture->Commander(0.0, 0.0, true);
		Phase = 1;
		Debut = T;
		return;
	}
	if (Phase == 1)
	{
		Voiture->Commander(T - Debut > 1.2 ? 1.0 : 0.0, 0.0, T - Debut <= 1.2);
		if (Voiture->VitesseKmh() >= Vitesses[EtapeParcours])
		{
			Phase = 2;
			Debut = T;
			AngleMax = GlisseMax = 0.0;
			LacetDepart = Voiture->GetActorRotation().Yaw;
		}
		return;
	}
	Voiture->Commander(1.0, 1.0, false);
	const FVector V = Voiture->GetVelocity().GetSafeNormal2D();
	const FVector Nez = Voiture->GetActorForwardVector().GetSafeNormal2D();
	AngleMax = FMath::Max(AngleMax, FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(V, Nez), -1.0, 1.0))));
	GlisseMax = FMath::Max(GlisseMax, Voiture->Derapage());
	if (T >= ProchainReleve)
	{
		ProchainReleve = T + 0.2;
		const FVector Ici = Voiture->GetActorLocation() / 100.0;
		Noter(FString::Printf(TEXT("  trajet %.1f s : (%.1f, %.1f) %.0f km/h cap %.0f"), T - Debut, Ici.X, Ici.Y, Voiture->VitesseKmh(), Voiture->GetActorRotation().Yaw));
	}
	if (T - Debut > 1.2)
	{
		Noter(FString::Printf(TEXT("virage a %.0f km/h : derive maxi %.1f degres, glisse maxi %.1f m/s, cap tourne de %.0f degres, vitesse finale %.0f km/h"),
			Vitesses[EtapeParcours], AngleMax, GlisseMax, FMath::Abs(FRotator::NormalizeAxis(Voiture->GetActorRotation().Yaw - LacetDepart)), Voiture->VitesseKmh()));
		EtapeParcours++;
		Phase = 0;
	}
}

// Essai sans clavier : monte dans la voiture, accélère, tourne, freine et note tout dans le journal.
void APalmeraJoueur::PiloteDeTest(float Dt)
{
	TempsPilote += Dt;
	if (bParcours)
	{
		ParcoursDeTest(Dt);
		return;
	}
	if (bEssaiVirage)
	{
		EssaiVirage(Dt);
		return;
	}
	if (!Voiture)
	{
		if (TempsPilote > 3.0 && Heros)
		{
			Monter();
		}
		return;
	}
	const double T = TempsPilote;
	const double Gaz = T < 11.0 ? 1.0 : (T < 13.0 ? -1.0 : 0.6);
	const double Volant = T < 13.0 ? 0.0 : (T < 15.0 ? 0.55 : 0.0);
	Voiture->Commander(Gaz, Volant, T > 12.0 && T < 13.0);
	if (T >= ProchainReleve)
	{
		ProchainReleve = T + 1.0;
		const FVector P = Voiture->GetActorLocation() / 100.0;
		UE_LOG(LogTemp, Display, TEXT("PALMERA pilote t=%.1f pos=(%.1f, %.1f, %.2f) vitesse=%.0f km/h regime=%.0f rapport=%d lacet=%.0f roulis=%.1f tangage=%.1f"),
			T, P.X, P.Y, P.Z, Voiture->VitesseKmh(), Voiture->Regime(), Voiture->Rapport(), Voiture->GetActorRotation().Yaw,
			Voiture->GetActorRotation().Roll, Voiture->GetActorRotation().Pitch);
	}
}

void APalmeraJoueur::CommandesAPied(float Dt)
{
	// Z et W, A et Q : les deux dispositions de clavier marchent, ainsi que les flèches
	double Avant = (Touche(EKeys::Z, EKeys::W, EKeys::Up) ? 1.0 : 0.0) - (Touche(EKeys::S, EKeys::Down) ? 1.0 : 0.0);
	double Cote = (Touche(EKeys::D, EKeys::Right) ? 1.0 : 0.0) - (Touche(EKeys::Q, EKeys::A, EKeys::Left) ? 1.0 : 0.0);
	Avant += GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	Cote += GetInputAnalogKeyState(EKeys::Gamepad_LeftX);

	const FRotator Lacet(0.0, GetControlRotation().Yaw, 0.0);
	const FRotationMatrix Repere(Lacet);
	Heros->AddMovementInput(Repere.GetUnitAxis(EAxis::X), float(FMath::Clamp(Avant, -1.0, 1.0)));
	Heros->AddMovementInput(Repere.GetUnitAxis(EAxis::Y), float(FMath::Clamp(Cote, -1.0, 1.0)));

	float Dx = 0.f, Dy = 0.f;
	GetInputMouseDelta(Dx, Dy);
	FRotator Vue = GetControlRotation();
	Vue.Yaw = FRotator::NormalizeAxis(Vue.Yaw + Dx * 2.4 + GetInputAnalogKeyState(EKeys::Gamepad_RightX) * 150.0 * Dt);
	Vue.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Vue.Pitch + Dy * 2.4 + GetInputAnalogKeyState(EKeys::Gamepad_RightY) * 110.0 * Dt), -65.0, 55.0);
	Vue.Roll = 0.0;
	SetControlRotation(Vue);

	Heros->Courir(Touche(EKeys::LeftShift, EKeys::Gamepad_LeftTrigger));
	if (Appui(EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom))
	{
		Heros->Jump();
	}
	if (Appui(EKeys::F, EKeys::Gamepad_FaceButton_Top))
	{
		Monter();
	}
}

void APalmeraJoueur::CommandesAuVolant(float Dt)
{
	double Gaz = (Touche(EKeys::Z, EKeys::W, EKeys::Up) ? 1.0 : 0.0) - (Touche(EKeys::S, EKeys::Down) ? 1.0 : 0.0);
	double Volant = (Touche(EKeys::D, EKeys::Right) ? 1.0 : 0.0) - (Touche(EKeys::Q, EKeys::A, EKeys::Left) ? 1.0 : 0.0);
	Gaz += GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) - GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis);
	Volant += GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const bool bFrein = Touche(EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom);
	Voiture->Commander(bCommandesGelees ? 0.0 : Gaz, bCommandesGelees ? 0.0 : Volant, bFrein || bCommandesGelees);

	// Regard libre autour de la voiture
	float Dx = 0.f, Dy = 0.f;
	GetInputMouseDelta(Dx, Dy);
	Voiture->TournerCamera(Dx * 2.4 + GetInputAnalogKeyState(EKeys::Gamepad_RightX) * 150.0 * Dt, Dy * 2.4 + GetInputAnalogKeyState(EKeys::Gamepad_RightY) * 110.0 * Dt);

	if (Heros && Appui(EKeys::F, EKeys::Gamepad_FaceButton_Top))
	{
		Descendre();
		return;
	}
	if (Appui(EKeys::L))
	{
		Voiture->AllumerPhares(!Voiture->PharesAllumes());
	}
	if (Appui(EKeys::V, EKeys::Gamepad_RightThumbstick))
	{
		Voiture->RecentrerCamera();
	}
	if (APalmeraSons* Sons = APalmeraSons::Lire(this))
	{
		if (Appui(EKeys::H))
		{
			Sons->Klaxon();
		}
		if (Appui(EKeys::C))
		{
			Sons->StationSuivante();
		}
	}
	if (Appui(EKeys::R) && Voiture->EstRetournee())
	{
		Voiture->Redresser();
	}
}

void APalmeraJoueur::PrendreVoiture(APalmeraVoiture* NouvelleVoiture)
{
	Voiture = NouvelleVoiture;
	Voiture->bCibleJoueur = true;
	Voiture->ActiverCamera(true);
	Possess(Voiture);
}

void APalmeraJoueur::LacherVoiture()
{
	if (Voiture)
	{
		UnPossess();
		Voiture = nullptr;
	}
}

void APalmeraJoueur::Monter()
{
	APalmeraVoiture* Proche = nullptr;
	double Meilleur = FMath::Square(bPiloteTest ? 5000.0 : 650.0);
	for (TActorIterator<APalmeraVoiture> It(GetWorld()); It; ++It)
	{
		if (It->bReservee || It->IsHidden() || It->EstHorsService())
		{
			continue;
		}
		const double D = FVector::DistSquared(It->GetActorLocation(), Heros->GetActorLocation());
		if (D < Meilleur)
		{
			Meilleur = D;
			Proche = *It;
		}
	}
	if (!Proche)
	{
		return;
	}
	Voiture = Proche;
	Voiture->ActiverPhysique(true);
	Voiture->bCibleJoueur = true;
	Voiture->ActiverCamera(true);
	Heros->ActiverCamera(false);
	Heros->Masquer(true);
	Heros->SetActorLocation(Voiture->GetActorLocation() + FVector(0, 0, 400));
	Possess(Voiture);
}

void APalmeraJoueur::Descendre()
{
	const FVector Sortie = Voiture->GetActorLocation() - Voiture->GetActorRightVector() * 200.0 + FVector(0, 0, 60);
	Voiture->Commander(0.0, 0.0, true);
	Voiture->bCibleJoueur = false;
	Voiture->ActiverCamera(false);
	Heros->SetActorLocationAndRotation(Sortie, FRotator(0.0, Voiture->GetActorRotation().Yaw, 0.0));
	Heros->Masquer(false);
	Heros->ActiverCamera(true);
	SetControlRotation(FRotator(-10.0, Voiture->GetActorRotation().Yaw, 0.0));
	Voiture = nullptr;
	Possess(Heros);
}
