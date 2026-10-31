#include "PalmeraHistoire.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PalmeraConfig.h"
#include "PalmeraHeros.h"
#include "PalmeraJoueur.h"
#include "PalmeraMonde.h"
#include "PalmeraPartie.h"
#include "PalmeraSons.h"
#include "PalmeraVoiture.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

using namespace Palmera;

namespace
{
	FVector2D LirePoint(const TArray<TSharedPtr<FJsonValue>>& V)
	{
		return V.Num() >= 2 ? FVector2D(V[0]->AsNumber(), V[1]->AsNumber()) : FVector2D::ZeroVector;
	}


	void LireLignes(const TSharedPtr<FJsonObject>& O, TArray<FReplique>& Sortie, int32& Numero)
	{
		const TArray<TSharedPtr<FJsonValue>>* V = nullptr;
		if (O->TryGetArrayField(TEXT("lignes"), V))
		{
			for (const TSharedPtr<FJsonValue>& L : *V)
			{
				const TArray<TSharedPtr<FJsonValue>>& Paire = L->AsArray();
				if (Paire.Num() >= 2)
				{
					FReplique R;
					R.Qui = Paire[0]->AsString();
					R.Texte = Paire[1]->AsString();
					R.Numero = Numero++;
					Sortie.Add(R);
				}
			}
		}
	}

	ESorteEtape LireSorte(const FString& T)
	{
		static const TMap<FString, ESorteEtape> Table = {
			{ TEXT("scene"), ESorteEtape::Scene }, { TEXT("appel"), ESorteEtape::Appel }, { TEXT("aller"), ESorteEtape::Aller },
			{ TEXT("parler"), ESorteEtape::Parler }, { TEXT("suivre"), ESorteEtape::Suivre }, { TEXT("detruire"), ESorteEtape::Detruire },
			{ TEXT("proteger"), ESorteEtape::Proteger }, { TEXT("semer"), ESorteEtape::Semer }, { TEXT("course"), ESorteEtape::Course },
			{ TEXT("infiltrer"), ESorteEtape::Infiltrer }, { TEXT("placer"), ESorteEtape::Placer }, { TEXT("attendre"), ESorteEtape::Attendre },
			{ TEXT("recherche"), ESorteEtape::Recherche }, { TEXT("fin"), ESorteEtape::Fin }
		};
		const ESorteEtape* S = Table.Find(T);
		UE_CLOG(!S, LogTemp, Warning, TEXT("PALMERA etape inconnue : %s"), *T);
		return S ? *S : ESorteEtape::Attendre;
	}
}

void FTrajet::Construire(const TArray<FVector2D>& Route, double DecalageDroite)
{
	Points.Reset();
	Cumul.Reset();
	Longueur = 0.0;
	S = 0.0;
	for (int32 I = 0; I < Route.Num(); I++)
	{
		// Décalage sur la voie de droite : perpendiculaire au tronçon
		const FVector2D Avant = Route[FMath::Max(I - 1, 0)];
		const FVector2D Apres = Route[FMath::Min(I + 1, Route.Num() - 1)];
		const FVector2D Dir = (Apres - Avant).GetSafeNormal();
		const FVector2D Droite(-Dir.Y, Dir.X);
		const FVector2D P = Route[I] + Droite * DecalageDroite;
		if (I > 0)
		{
			Longueur += FVector2D::Distance(Points.Last(), P);
		}
		Points.Add(P);
		Cumul.Add(Longueur);
	}
}

FVector2D FTrajet::Position(double Distance, double& Cap) const
{
	Cap = 0.0;
	if (Points.Num() < 2)
	{
		return Points.Num() == 1 ? Points[0] : FVector2D::ZeroVector;
	}
	Distance = FMath::Clamp(Distance, 0.0, Longueur);
	int32 I = 0;
	while (I < Points.Num() - 2 && Cumul[I + 1] < Distance)
	{
		I++;
	}
	const double Segment = FMath::Max(Cumul[I + 1] - Cumul[I], 0.01);
	const double T = (Distance - Cumul[I]) / Segment;
	const FVector2D Dir = (Points[I + 1] - Points[I]).GetSafeNormal();
	Cap = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
	return FMath::Lerp(Points[I], Points[I + 1], T);
}

APalmeraHistoire::APalmeraHistoire()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.bTickEvenWhenPaused = false;
}

APalmeraHistoire* APalmeraHistoire::Lire(const UObject* Contexte)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (Monde)
	{
		for (TActorIterator<APalmeraHistoire> It(Monde); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

APalmeraPartie* APalmeraHistoire::Partie() const { return APalmeraPartie::Lire(this); }
APalmeraJoueur* APalmeraHistoire::Joueur() const { return Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController()); }

FVector APalmeraHistoire::PositionJoueur() const
{
	const APalmeraJoueur* J = Joueur();
	return (J && J->GetPawn()) ? J->GetPawn()->GetActorLocation() : FVector::ZeroVector;
}

bool APalmeraHistoire::Charger()
{
	FString Texte;
	const FString Chemin = FPaths::ProjectContentDir() / TEXT("Histoire/histoire.json");
	if (!FFileHelper::LoadFileToString(Texte, *Chemin))
	{
		UE_LOG(LogTemp, Warning, TEXT("PALMERA histoire absente : %s"), *Chemin);
		return false;
	}
	TSharedPtr<FJsonObject> Racine;
	const TSharedRef<TJsonReader<>> Lecteur = TJsonReaderFactory<>::Create(Texte);
	if (!FJsonSerializer::Deserialize(Lecteur, Racine) || !Racine.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PALMERA histoire illisible : %s"), *Lecteur->GetErrorMessage());
		return false;
	}

	const TSharedPtr<FJsonObject>* Perso = nullptr;
	if (Racine->TryGetObjectField(TEXT("personnages"), Perso))
	{
		for (const auto& Paire : (*Perso)->Values)
		{
			const TSharedPtr<FJsonObject> O = Paire.Value->AsObject();
			FFichePNJ F;
			F.Id = Paire.Key;
			F.Nom = O->GetStringField(TEXT("nom"));
			O->TryGetBoolField(TEXT("femme"), F.bFemme);
			O->TryGetStringField(TEXT("modele"), F.Modele);
			O->TryGetStringField(TEXT("voix"), F.Voix);
			double H = 1.0;
			if (O->TryGetNumberField(TEXT("hauteur"), H))
			{
				F.Hauteur = float(H);
			}
			FichesParId.Add(F.Id, F);
			Fiches.Add(F);
		}
	}

	int32 Numero = 0;
	for (const TSharedPtr<FJsonValue>& VC : Racine->GetArrayField(TEXT("chapitres")))
	{
		const TSharedPtr<FJsonObject> OC = VC->AsObject();
		FChapitre C;
		C.Titre = OC->GetStringField(TEXT("titre"));
		for (const TSharedPtr<FJsonValue>& VE : OC->GetArrayField(TEXT("etapes")))
		{
			const TSharedPtr<FJsonObject> O = VE->AsObject();
			FEtapeHistoire E;
			E.Sorte = LireSorte(O->GetStringField(TEXT("type")));
			O->TryGetStringField(TEXT("consigne"), E.Consigne);
			O->TryGetStringField(TEXT("pnj"), E.Pnj);
			O->TryGetStringField(TEXT("si"), E.Si);
			O->TryGetStringField(TEXT("sinon"), E.SiNon);
			O->TryGetStringField(TEXT("raison"), E.Raison);
			O->TryGetNumberField(TEXT("rayon"), E.Rayon);
			O->TryGetNumberField(TEXT("temps"), E.Temps);
			O->TryGetNumberField(TEXT("vitesse"), E.Vitesse);
			O->TryGetNumberField(TEXT("vie"), E.Vie);
			O->TryGetNumberField(TEXT("heure"), E.Heure);
			O->TryGetNumberField(TEXT("pluie"), E.Pluie);
			O->TryGetNumberField(TEXT("nombre"), E.Nombre);
			O->TryGetNumberField(TEXT("argent"), E.Argent);
			O->TryGetNumberField(TEXT("niveau"), E.Niveau);
			O->TryGetBoolField(TEXT("chef"), E.bChef);
			O->TryGetBoolField(TEXT("reprise"), E.bReprise);
			O->TryGetBoolField(TEXT("cacher"), E.bCacher);
			O->TryGetBoolField(TEXT("apied"), E.bAPied);
			const TArray<TSharedPtr<FJsonValue>>* P = nullptr;
			if (O->TryGetArrayField(TEXT("point"), P))
			{
				E.Point = LirePoint(*P);
			}
			if (O->TryGetArrayField(TEXT("route"), P))
			{
				for (const TSharedPtr<FJsonValue>& R : *P)
				{
					E.Route.Add(LirePoint(R->AsArray()));
				}
			}
			LireLignes(O, E.Lignes, Numero);
			if (O->TryGetArrayField(TEXT("choix"), P))
			{
				for (const TSharedPtr<FJsonValue>& VO : *P)
				{
					const TSharedPtr<FJsonObject> OChoix = VO->AsObject();
					FChoixHistoire Ch;
					Ch.Texte = OChoix->GetStringField(TEXT("texte"));
					OChoix->TryGetStringField(TEXT("drapeau"), Ch.Drapeau);
					OChoix->TryGetNumberField(TEXT("argent"), Ch.Argent);
					LireLignes(OChoix, Ch.Suite, Numero);
					E.Choix.Add(Ch);
				}
			}
			C.Etapes.Add(E);
		}
		Chapitres.Add(C);
	}

	PremierNumeroCassette = Numero;
	for (const TSharedPtr<FJsonValue>& V : Racine->GetArrayField(TEXT("cassettes")))
	{
		TextesCassettes.Add(V->AsString());
	}
	UE_LOG(LogTemp, Display, TEXT("PALMERA histoire : %d chapitres, %d personnages, %d repliques, %d cassettes"),
		Chapitres.Num(), Fiches.Num(), Numero, TextesCassettes.Num());
	return Chapitres.Num() > 0;
}

void APalmeraHistoire::BeginPlay()
{
	Super::BeginPlay();

	CameraScene = GetWorld()->SpawnActor<ACameraActor>();
	CameraScene->GetCameraComponent()->bConstrainAspectRatio = false;
	CameraScene->GetCameraComponent()->SetFieldOfView(38.0f);
	if (!Charger())
	{
		SetActorTickEnabled(false);
		return;
	}
	if (const APalmeraPartie* P = Partie())
	{
		Chapitre = P->ChapitreSauve;
		Drapeaux = P->DrapeauxSauves;
	}
	FParse::Value(FCommandLine::Get(), TEXT("-chapitre="), Chapitre);
	bTestAuto = FParse::Param(FCommandLine::Get(), TEXT("testhistoire"));
	bTestPas = FParse::Param(FCommandLine::Get(), TEXT("testpas"));
	if (Chapitre < Chapitres.Num())
	{
		DemarrerChapitre();
	}
}

void APalmeraHistoire::AllerAuChapitre(int32 Index)
{
	Nettoyer();
	Chapitre = Index;
	if (Chapitre < Chapitres.Num())
	{
		DemarrerChapitre();
	}
}

const FEtapeHistoire* APalmeraHistoire::EtapeActuelle() const
{
	return Chapitres.IsValidIndex(Chapitre) && Chapitres[Chapitre].Etapes.IsValidIndex(IndexEtape) ? &Chapitres[Chapitre].Etapes[IndexEtape] : nullptr;
}

FString APalmeraHistoire::NomChapitre() const
{
	return Chapitres.IsValidIndex(Chapitre) ? Chapitres[Chapitre].Titre : FString(TEXT("Histoire terminée"));
}

FString APalmeraHistoire::Resume() const
{
	const FEtapeHistoire* E = EtapeActuelle();
	return E ? E->Consigne : FString();
}

bool APalmeraHistoire::EstOccupee() const
{
	const FEtapeHistoire* E = EtapeActuelle();
	if (!E || EstTerminee())
	{
		return false;
	}
	// Entre deux chapitres, le joueur est libre : seule une étape « parler » sans urgence le laisse faire
	return E->Sorte != ESorteEtape::Parler;
}

void APalmeraHistoire::DemarrerChapitre()
{
	IndexEtape = -1;
	Reprise = 0;
	if (APalmeraPartie* P = Partie())
	{
		P->ChapitreSauve = Chapitre;
		P->DrapeauxSauves = Drapeaux;
		P->Sauvegarder();
	}
	EtapeSuivante();
}

void APalmeraHistoire::EtapeSuivante()
{
	IndexEtape++;
	if (!Chapitres.IsValidIndex(Chapitre))
	{
		return;
	}
	if (IndexEtape >= Chapitres[Chapitre].Etapes.Num())
	{
		Nettoyer();
		Chapitre++;
		if (Chapitre < Chapitres.Num())
		{
			DemarrerChapitre();
		}
		else if (APalmeraPartie* P = Partie())
		{
			P->ChapitreSauve = Chapitre;
			P->DrapeauxSauves = Drapeaux;
			P->Objectif.Empty();
			P->bCibleActive = false;
			P->Sauvegarder();
		}
		return;
	}
	DemarrerEtape();
}

void APalmeraHistoire::Nettoyer()
{
	for (FEnnemi& E : Ennemis)
	{
		if (E.Voiture && !E.Voiture->bCibleJoueur)
		{
			E.Voiture->Destroy();
		}
	}
	Ennemis.Reset();
	for (FGarde& G : Gardes)
	{
		if (G.Corps)
		{
			G.Corps->Destroy();
		}
	}
	Gardes.Reset();
	for (APalmeraVoiture* R : Rivaux)
	{
		if (R)
		{
			R->Destroy();
		}
	}
	Rivaux.Reset();
	TrajetsRivaux.Reset();
	if (VoitureTrajet)
	{
		VoitureTrajet->Destroy();
		VoitureTrajet = nullptr;
	}
	Detection = VieAlliee = VieCible = -1.0;
	NomCible.Empty();
	Invite.Empty();
	if (APalmeraPartie* P = Partie())
	{
		P->bCibleActive = false;
		P->Chrono = -1.0;
		P->CacherBalise();
	}
}

void APalmeraHistoire::Echouer(const FString& Raison)
{
	if (APalmeraPartie* P = Partie())
	{
		P->Annoncer(FString::Printf(TEXT("ÉCHEC : %s"), *Raison), 5.0);
		P->Recherche = 0;
		P->Chaleur = 0.0;
	}
	Lignes.Reset();
	IndexLigne = -1;
	Nettoyer();
	IndexEtape = Reprise - 1;
	EtapeSuivante();
}

void APalmeraHistoire::DemarrerEtape()
{
	const FEtapeHistoire& E = Chapitres[Chapitre].Etapes[IndexEtape];
	if ((!E.Si.IsEmpty() && !Drapeaux.Contains(E.Si)) || (!E.SiNon.IsEmpty() && Drapeaux.Contains(E.SiNon)))
	{
		EtapeSuivante();
		return;
	}
	if (E.bReprise)
	{
		Reprise = IndexEtape;
	}
	if (IndexEtape == 0)
	{
		TitreEcran = TEXT("PALMERA BAY");
		SousTitreEcran = Chapitres[Chapitre].Titre;
		FinTitre = GetWorld()->GetTimeSeconds() + 5.0;
	}

	APalmeraPartie* P = Partie();
	for (TActorIterator<APalmeraMonde> It(GetWorld()); It; ++It)
	{
		if (E.Heure >= 0.0)
		{
			It->Heure = E.Heure;
		}
		if (E.Pluie >= 0.0)
		{
			It->Pluie = It->PluieCible = E.Pluie;
			It->bMeteoAuto = false;
		}
	}
	if (E.Argent != 0 && E.Sorte != ESorteEtape::Fin)
	{
		P->Gagner(E.Argent);
		P->Annoncer(FString::Printf(TEXT("%+d $"), E.Argent), 3.0);
	}

	TempsEtape = 0.0;
	TempsTropLoin = 0.0;
	TempsHorsVue = 0.0;
	Invite.Empty();
	if (!E.Consigne.IsEmpty() || E.Sorte != ESorteEtape::Appel)
	{
		P->Objectif = E.Consigne;
	}
	P->Chrono = E.Temps;

	switch (E.Sorte)
	{
	case ESorteEtape::Scene:
		if (APalmeraPNJ* Avec = PNJ(E.Pnj))
		{
			InterlocuteurScene = Avec;
			P->bCibleActive = false;
			P->CacherBalise();
			P->Chrono = -1.0;
			bScene = true;
			Joueur()->CommencerScene(Avec, CameraScene);
		}
		bChoixFait = false;
		LancerLignes(E.Lignes, false);
		break;

	case ESorteEtape::Appel:
		LancerLignes(E.Lignes, true);
		break;

	case ESorteEtape::Aller:
	case ESorteEtape::Infiltrer:
		P->bCibleActive = true;
		P->Cible = E.Point;
		P->PlacerBalise(E.Point, E.Rayon, FLinearColor(1.f, 0.72f, 0.2f));
		if (E.Sorte == ESorteEtape::Infiltrer)
		{
			Detection = 0.0;
			FRandomStream H(IndexEtape * 31 + Chapitre * 7 + 1);
			for (int32 I = 0; I < E.Nombre; I++)
			{
				const double Angle = H.FRandRange(0.f, 6.28f);
				const double R = 7.0 + 4.0 * I;
				const FVector2D A = E.Point + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R;
				const FVector2D B = E.Point + FVector2D(FMath::Cos(Angle + 1.7), FMath::Sin(Angle + 1.7)) * (R + 3.0);
				FActorSpawnParameters Regles;
				Regles.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				APalmeraPNJ* C = GetWorld()->SpawnActor<APalmeraPNJ>(EnMetres(A.X, A.Y, 1.1), FRotator::ZeroRotator, Regles);
				C->Configurer(FicheDe(TEXT("garde")));
				FGarde G;
				G.Corps = C;
				G.A = A;
				G.B = B;
				Gardes.Add(G);
			}
		}
		break;

	case ESorteEtape::Parler:
		if (APalmeraPNJ* Avec = PNJ(E.Pnj))
		{
			const FVector Pos = Avec->GetActorLocation() / Metre;
			P->bCibleActive = true;
			P->Cible = FVector2D(Pos.X, Pos.Y);
			P->PlacerBalise(FVector2D(Pos.X, Pos.Y), 2.2, FLinearColor(0.3f, 0.85f, 1.f));
		}
		break;

	case ESorteEtape::Placer:
		if (E.bCacher)
		{
			if (TObjectPtr<APalmeraPNJ>* T = Personnages.Find(E.Pnj))
			{
				(*T)->Destroy();
				Personnages.Remove(E.Pnj);
			}
		}
		else if (APalmeraPNJ* Q = PNJ(E.Pnj))
		{
			Q->SetActorLocation(EnMetres(E.Point.X, E.Point.Y, 1.1), false, nullptr, ETeleportType::ResetPhysics);
		}
		EtapeSuivante();
		return;

	case ESorteEtape::Recherche:
		P->Chaleur = E.Niveau * 20.0;
		P->Recherche = E.Niveau;
		EtapeSuivante();
		return;

	case ESorteEtape::Suivre:
		if (E.Route.Num() >= 2)
		{
			Trajet.Construire(E.Route, 1.75);
			Trajet.Vitesse = E.Vitesse;
			double Cap = 0.0;
			const FVector2D Depart = Trajet.Position(0.0, Cap);
			VoitureTrajet = CreerVoiture(3, FLinearColor(0.02f, 0.25f, 0.3f), Depart, Cap, false);
			P->bCibleActive = true;
			P->Cible = Depart;
		}
		break;

	case ESorteEtape::Proteger:
		if (E.Route.Num() >= 2)
		{
			Trajet.Construire(E.Route, 1.75);
			Trajet.Vitesse = E.Vitesse;
			double Cap = 0.0;
			const FVector2D Depart = Trajet.Position(0.0, Cap);
			VoitureTrajet = CreerVoiture(1, FLinearColor(0.8f, 0.62f, 0.05f), Depart, Cap, false);
			VieAllieeBrute = E.Vie;
			VieAlliee = 1.0;
			SortirEnnemis(E.Nombre, 70.0, false);
			P->bCibleActive = true;
			P->Cible = E.Route.Last();
		}
		break;

	case ESorteEtape::Detruire:
		SortirEnnemis(E.Nombre, E.Vie, E.bChef);
		if (E.bChef)
		{
			NomCible = TEXT("La Murène");
		}
		break;

	case ESorteEtape::Semer:
		SortirEnnemis(E.Nombre, E.Vie, false);
		if (E.Rayon > 0.0 && !E.Point.IsZero())
		{
			P->bCibleActive = true;
			P->Cible = E.Point;
			P->PlacerBalise(E.Point, E.Rayon, FLinearColor(1.f, 0.72f, 0.2f));
		}
		break;

	case ESorteEtape::Course:
		PointCourse = 0;
		if (E.Route.Num() > 0)
		{
			P->bCibleActive = true;
			P->Cible = E.Route[0];
			P->PlacerBalise(E.Route[0], E.Rayon, FLinearColor(1.f, 0.72f, 0.2f));
			TArray<FVector2D> Depart;
			Depart.Add(FVector2D(PositionJoueur().X / Metre, PositionJoueur().Y / Metre));
			Depart.Append(E.Route);
			for (int32 I = 0; I < E.Nombre; I++)
			{
				FTrajet T;
				T.Construire(Depart, I == 0 ? -4.0 : 4.0);
				T.Vitesse = E.Vitesse * (1.0 - I * 0.04);
				double Cap = 0.0;
				const FVector2D Pos = T.Position(14.0 * (I + 1), Cap);
				T.S = 14.0 * (I + 1);
				TrajetsRivaux.Add(T);
				APalmeraVoiture* R = CreerVoiture(-1, I == 0 ? FLinearColor(0.35f, 0.02f, 0.5f) : FLinearColor(0.02f, 0.3f, 0.32f), Pos, Cap, false);
				Rivaux.Add(R);
			}
		}
		break;

	case ESorteEtape::Fin:
		P->Gagner(E.Argent);
		TitreEcran = E.Consigne;
		SousTitreEcran = TEXT("FIN — mais la ville reste à toi");
		FinTitre = GetWorld()->GetTimeSeconds() + 9.0;
		P->Objectif.Empty();
		EtapeSuivante();
		return;

	default:
		break;
	}
}

FFichePNJ APalmeraHistoire::FicheDe(const FString& Id) const
{
	if (const FFichePNJ* F = FichesParId.Find(Id))
	{
		return *F;
	}
	FFichePNJ Vide;
	Vide.Id = Id;
	Vide.Nom = Id;
	return Vide;
}

APalmeraPNJ* APalmeraHistoire::PNJ(const FString& Id)
{
	if (Id.IsEmpty())
	{
		return nullptr;
	}
	if (TObjectPtr<APalmeraPNJ>* T = Personnages.Find(Id))
	{
		return *T;
	}
	FActorSpawnParameters Regles;
	Regles.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FEtapeHistoire* E = EtapeActuelle();
	const FVector2D Pos = E ? E->Point : FVector2D::ZeroVector;
	APalmeraPNJ* C = GetWorld()->SpawnActor<APalmeraPNJ>(EnMetres(Pos.X, Pos.Y, 1.1), FRotator::ZeroRotator, Regles);
	C->Configurer(FicheDe(Id));
	Personnages.Add(Id, C);
	return C;
}

void APalmeraHistoire::LancerLignes(const TArray<FReplique>& Liste, bool bAutomatique)
{
	Lignes = Liste;
	IndexLigne = Lignes.Num() > 0 ? 0 : -1;
	TempsLigne = 0.0;
	bAuto = bAutomatique;
	if (IndexLigne < 0)
	{
		FinDesLignes();
		return;
	}
	CadrerScene();
	Dire(Lignes[0]);
}

void APalmeraHistoire::Dire(const FReplique& Ligne)
{
	if (APalmeraSons* Sons = APalmeraSons::Lire(this))
	{
		Sons->DireVoix(Ligne.Numero, FicheDe(Ligne.Qui).Hauteur);
	}
}

void APalmeraHistoire::LigneSuivante()
{
	IndexLigne++;
	TempsLigne = 0.0;
	if (Lignes.IsValidIndex(IndexLigne))
	{
		CadrerScene();
		Dire(Lignes[IndexLigne]);
	}
	else
	{
		FinDesLignes();
	}
}

void APalmeraHistoire::FinDesLignes()
{
	Lignes.Reset();
	IndexLigne = -1;
	const FEtapeHistoire* E = EtapeActuelle();

	// Répliques d'une cassette écoutée entre deux étapes : l'étape n'est pas concernée
	if (AppelsEnAttente.Num() > 0)
	{
		TArray<FReplique> Suite = AppelsEnAttente[0];
		AppelsEnAttente.RemoveAt(0);
		LancerLignes(Suite, true);
		return;
	}
	if (!E || (E->Sorte != ESorteEtape::Scene && E->Sorte != ESorteEtape::Appel))
	{
		return;
	}
	if (E->Sorte == ESorteEtape::Scene && E->Choix.Num() > 0 && !bChoixFait)
	{
		TextesChoix.Reset();
		for (const FChoixHistoire& C : E->Choix)
		{
			TextesChoix.Add(C.Texte);
		}
		Selection = 0;
		bChoixOuvert = true;
		return;
	}
	if (bScene)
	{
		bScene = false;
		Joueur()->FinirScene();
	}
	EtapeSuivante();
}

void APalmeraHistoire::Avancer()
{
	if (bChoixOuvert)
	{
		const FEtapeHistoire* E = EtapeActuelle();
		bChoixOuvert = false;
		bChoixFait = true;
		if (E && E->Choix.IsValidIndex(Selection))
		{
			const FChoixHistoire& C = E->Choix[Selection];
			if (!C.Drapeau.IsEmpty())
			{
				Drapeaux.AddUnique(C.Drapeau);
			}
			if (C.Argent != 0)
			{
				Partie()->Gagner(C.Argent);
			}
			LancerLignes(C.Suite, false);
		}
		else
		{
			FinDesLignes();
		}
		return;
	}
	if (!Lignes.IsValidIndex(IndexLigne) || bAuto)
	{
		return;
	}
	// Premier appui : affiche toute la réplique ; second appui : passe à la suivante
	const double Complet = Lignes[IndexLigne].Texte.Len() * 0.022;
	if (TempsLigne < Complet)
	{
		TempsLigne = Complet;
	}
	else
	{
		LigneSuivante();
	}
}

void APalmeraHistoire::DeplacerSelection(int32 Sens)
{
	if (bChoixOuvert && TextesChoix.Num() > 0)
	{
		Selection = (Selection + Sens + TextesChoix.Num()) % TextesChoix.Num();
	}
}

FString APalmeraHistoire::NomParleur() const
{
	return Lignes.IsValidIndex(IndexLigne) ? FicheDe(Lignes[IndexLigne].Qui).Nom : FString();
}

FLinearColor APalmeraHistoire::CouleurParleur() const
{
	if (!Lignes.IsValidIndex(IndexLigne))
	{
		return FLinearColor::White;
	}
	const FString& Qui = Lignes[IndexLigne].Qui;
	if (Qui == TEXT("sam"))
	{
		return FLinearColor(0.45f, 0.85f, 1.f);
	}
	if (Qui == TEXT("marius"))
	{
		return FLinearColor(0.75f, 0.95f, 0.7f);
	}
	return FLinearColor(1.f, 0.72f, 0.3f);
}

FString APalmeraHistoire::TexteAffiche() const
{
	if (!Lignes.IsValidIndex(IndexLigne))
	{
		return FString();
	}
	const FString& T = Lignes[IndexLigne].Texte;
	return T.Left(FMath::Min(T.Len(), int32(TempsLigne / 0.022) + 1));
}

void APalmeraHistoire::CadrerScene()
{
	if (!bScene || !InterlocuteurScene || !Lignes.IsValidIndex(IndexLigne))
	{
		return;
	}
	const APalmeraHeros* Heros = Joueur()->LireHeros();
	if (!Heros)
	{
		return;
	}
	// Plan par-dessus l'épaule de celui qui écoute, vers celui qui parle
	const bool bSamParle = Lignes[IndexLigne].Qui == TEXT("sam");
	const FVector Parle = (bSamParle ? Heros->GetActorLocation() : InterlocuteurScene->GetActorLocation()) + FVector(0, 0, 62);
	const FVector Ecoute = (bSamParle ? InterlocuteurScene->GetActorLocation() : Heros->GetActorLocation()) + FVector(0, 0, 62);
	const FVector Axe = (Parle - Ecoute).GetSafeNormal2D();
	const FVector Cote = FVector::CrossProduct(FVector::UpVector, Axe);
	const FVector Haut(0, 0, 12);
	const FVector Plans[] = { Ecoute - Axe * 95.0 + Cote * 42.0 + Haut, Ecoute - Axe * 95.0 - Cote * 42.0 + Haut, Ecoute - Axe * 20.0 + Cote * 46.0 + Haut };

	// Si un lampadaire ou un palmier coupe le plan, on passe sur l'autre épaule, puis au plus près
	FCollisionQueryParams Regles(SCENE_QUERY_STAT(PlanScene), false);
	Regles.AddIgnoredActor(Heros);
	Regles.AddIgnoredActor(InterlocuteurScene);
	FVector Position = Plans[0];
	for (const FVector& Plan : Plans)
	{
		FHitResult Obstacle;
		if (!GetWorld()->SweepSingleByChannel(Obstacle, Plan, Parle, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(28.f), Regles))
		{
			Position = Plan;
			break;
		}
	}
	CameraScene->SetActorLocationAndRotation(Position, (Parle - Position).Rotation());
}

APalmeraVoiture* APalmeraHistoire::CreerVoiture(int32 Modele, const FLinearColor& Peinture, const FVector2D& Position, double Cap, bool bPhysique)
{
	const FTransform Place(FRotator(0.0, Cap, 0.0), EnMetres(Position.X, Position.Y, 1.0));
	APalmeraVoiture* V = GetWorld()->SpawnActorDeferred<APalmeraVoiture>(APalmeraVoiture::StaticClass(), Place);
	FFicheVoiture F = Modele < 0 ? APalmeraVoiture::FicheSport() : APalmeraVoiture::FicheCivile(Modele);
	if (bPhysique)
	{
		F.Puissance = FMath::Max(F.Puissance, 210000.0);
		F.ForceMax = FMath::Max(F.ForceMax, 11000.0);
	}
	F.Peinture = Peinture;
	V->Configurer(F);
	V->bReservee = true;
	V->FinishSpawning(Place);
	V->ActiverPhysique(bPhysique);
	if (!bPhysique)
	{
		V->SetActorLocation(EnMetres(Position.X, Position.Y) + FVector(0, 0, F.CentreBoite.Z), false, nullptr, ETeleportType::ResetPhysics);
	}
	return V;
}

FVector2D APalmeraHistoire::PlaceLoinDuJoueur(double Min, double Max) const
{
	const FVector P = PositionJoueur() / Metre;
	for (int32 Essai = 0; Essai < 40; Essai++)
	{
		const int32 Kx = FMath::Clamp(FMath::RoundToInt(P.X / Pas) + FMath::RandRange(-3, 3), RueMin, RueMax);
		const int32 Ky = FMath::Clamp(FMath::RoundToInt(P.Y / Pas) + FMath::RandRange(-3, 3), RueMin, RueMax);
		const FVector2D Q(Kx * Pas - 1.75, Ky * Pas + 30.0);
		const double D = FVector2D::Distance(Q, FVector2D(P.X, P.Y));
		if (D >= Min && D <= Max)
		{
			return Q;
		}
	}
	return FVector2D(P.X - 120.0, P.Y);
}

void APalmeraHistoire::SortirEnnemis(int32 Nombre, double Vie, bool bChef)
{
	for (int32 I = 0; I < Nombre; I++)
	{
		const bool bBoss = bChef && I == 0;
		const FVector2D Place = PlaceLoinDuJoueur(90.0, 240.0);
		APalmeraVoiture* V = CreerVoiture(bBoss ? -1 : (I % 2 == 0 ? 2 : 4), bBoss ? FLinearColor(0.35f, 0.02f, 0.5f) : FLinearColor(0.015f, 0.17f, 0.18f), Place, 90.0, true);
		V->bEnnemie = true;
		V->DefinirVie(bBoss ? Vie : FMath::Min(Vie, 80.0));
		V->AllumerPhares(true);
		FEnnemi E;
		E.Voiture = V;
		Ennemis.Add(E);
	}
}

int32 APalmeraHistoire::EnnemisEnEtat() const
{
	int32 N = 0;
	for (const FEnnemi& E : Ennemis)
	{
		if (E.Voiture && !E.Voiture->EstHorsService())
		{
			N++;
		}
	}
	return N;
}

void APalmeraHistoire::ActualiserEnnemis(double Dt, const FVector& Cible, const AActor* ActeurCible)
{
	for (FEnnemi& E : Ennemis)
	{
		if (!E.Voiture)
		{
			continue;
		}
		if (E.Voiture->EstHorsService())
		{
			E.Voiture->Commander(0.0, 0.0, true);
			continue;
		}
		Palmera::PiloterVers(E.Voiture, Cible, ActeurCible, E.Pilote, Dt, 0.0);
		// Une voiture retournée ou perdue revient dans la poursuite
		if (E.Voiture->EstRetournee())
		{
			E.Voiture->Redresser();
		}
	}
}

void APalmeraHistoire::AvancerTrajet(FTrajet& T, APalmeraVoiture* V, double Dt)
{
	T.S = FMath::Min(T.S + T.Vitesse * Dt, T.Longueur);
	double Cap = 0.0;
	const FVector2D P = T.Position(T.S, Cap);
	const FRotator Actuel = V->GetActorRotation();
	const FRotator Voulu(0.0, FMath::FInterpTo(0.0, FMath::FindDeltaAngleDegrees(Actuel.Yaw, Cap), Dt, 5.0) + Actuel.Yaw, 0.0);
	V->SetActorLocationAndRotation(EnMetres(P.X, P.Y) + FVector(0, 0, V->LireFiche().CentreBoite.Z), Voulu, false, nullptr, ETeleportType::TeleportPhysics);
	V->AnimerRoues(T.Vitesse, Dt);
}

void APalmeraHistoire::ActualiserGardes(double Dt)
{
	const APalmeraJoueur* J = Joueur();
	const APawn* Moi = J ? J->GetPawn() : nullptr;
	if (!Moi)
	{
		return;
	}
	bool bVu = false;
	double Proche = 1.0;
	for (FGarde& G : Gardes)
	{
		if (!G.Corps)
		{
			continue;
		}
		const FVector Pos = G.Corps->GetActorLocation();
		const FVector2D But = (G.bAller ? G.B : G.A) * Metre;
		const FVector VersBut = FVector(But.X - Pos.X, But.Y - Pos.Y, 0.0);
		if (G.Pause > 0.0)
		{
			G.Pause -= Dt;
		}
		else if (VersBut.Size() < 80.0)
		{
			G.bAller = !G.bAller;
			G.Pause = 1.4;
		}
		else
		{
			G.Corps->Marcher(VersBut.GetSafeNormal(), 130.0f);
		}

		// Champ de vision : 15 m, en cône, sans obstacle entre eux
		const FVector VersMoi = Moi->GetActorLocation() - Pos;
		const double D = VersMoi.Size2D() / Metre;
		if (D < 15.0 && FVector::DotProduct(VersMoi.GetSafeNormal2D(), G.Corps->GetActorForwardVector()) > 0.5)
		{
			FHitResult Touche;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(VueGarde), false, G.Corps);
			Q.AddIgnoredActor(Moi);
			if (!GetWorld()->LineTraceSingleByChannel(Touche, Pos + FVector(0, 0, 60), Moi->GetActorLocation() + FVector(0, 0, 40), ECC_Visibility, Q))
			{
				bVu = true;
				Proche = FMath::Min(Proche, D / 15.0);
			}
		}
	}
	Detection = FMath::Clamp(Detection + (bVu ? Dt * (1.5 - Proche) * 0.9 : -Dt * 0.22), 0.0, 1.0);
}

void APalmeraHistoire::JouerCassette(int32 Rang)
{
	if (!TextesCassettes.IsValidIndex(Rang))
	{
		return;
	}
	FReplique R;
	R.Qui = TEXT("marius");
	R.Texte = TextesCassettes[Rang];
	R.Numero = PremierNumeroCassette + Rang;
	TArray<FReplique> Liste = { R };
	if (DialogueActif())
	{
		AppelsEnAttente.Add(Liste);
	}
	else
	{
		LancerLignes(Liste, true);
	}
}

void APalmeraHistoire::ActualiserEtape(double Dt)
{
	const FEtapeHistoire* E = EtapeActuelle();
	APalmeraPartie* P = Partie();
	APalmeraJoueur* J = Joueur();
	if (!E || !P || !J || !J->GetPawn())
	{
		return;
	}
	TempsEtape += Dt;
	const FVector Pos3 = J->GetPawn()->GetActorLocation() / Metre;
	const FVector2D Pos(Pos3.X, Pos3.Y);
	const FString RaisonEchec = E->Raison.IsEmpty() ? FString(TEXT("temps écoulé")) : E->Raison;

	if (P->Chrono >= 0.0)
	{
		P->Chrono -= Dt;
		if (P->Chrono <= 0.0)
		{
			Echouer(RaisonEchec);
			return;
		}
	}
	const APalmeraVoiture* Mienne = J->VoitureActuelle();
	if (Mienne && Mienne->EstHorsService() && (E->Sorte == ESorteEtape::Detruire || E->Sorte == ESorteEtape::Proteger || E->Sorte == ESorteEtape::Semer || E->Sorte == ESorteEtape::Suivre))
	{
		Echouer(TEXT("ta voiture est détruite"));
		return;
	}

	switch (E->Sorte)
	{
	case ESorteEtape::Aller:
		if (FVector2D::Distance(Pos, E->Point) < E->Rayon && (!E->bAPied || !Mienne))
		{
			EtapeSuivante();
		}
		break;

	case ESorteEtape::Parler:
		if (APalmeraPNJ* Avec = PNJ(E->Pnj))
		{
			const FVector Q = Avec->GetActorLocation() / Metre;
			const double D = FVector2D::Distance(Pos, FVector2D(Q.X, Q.Y));
			Invite = D < 6.0 ? FString::Printf(TEXT("E : parler à %s"), *Avec->LireFiche().Nom) : FString();
			if (D < 6.0 && J->DemandeAction())
			{
				Invite.Empty();
				EtapeSuivante();
			}
		}
		break;

	case ESorteEtape::Suivre:
		if (VoitureTrajet)
		{
			AvancerTrajet(Trajet, VoitureTrajet, Dt);
			const FVector Q = VoitureTrajet->GetActorLocation() / Metre;
			P->Cible = FVector2D(Q.X, Q.Y);
			const double D = FVector2D::Distance(Pos, P->Cible);
			Invite = D < 25.0 ? FString(TEXT("Trop près, recule !")) : (D > 120.0 ? FString(TEXT("Tu le perds !")) : FString());
			TempsTropLoin = (D > 170.0 || D < 12.0) ? TempsTropLoin + Dt : 0.0;
			if (TempsTropLoin > 3.0)
			{
				Echouer(D < 12.0 ? FString(TEXT("il t'a repéré")) : FString(TEXT("tu l'as perdu")));
				return;
			}
			if (Trajet.Fini())
			{
				EtapeSuivante();
			}
		}
		break;

	case ESorteEtape::Detruire:
		ActualiserEnnemis(Dt, J->GetPawn()->GetActorLocation(), J->GetPawn());
		if (E->bChef && Ennemis.Num() > 0 && Ennemis[0].Voiture)
		{
			VieCible = Ennemis[0].Voiture->LireVie() / Ennemis[0].Voiture->LireVieMax();
		}
		if ((E->bChef && Ennemis.Num() > 0 && Ennemis[0].Voiture && Ennemis[0].Voiture->EstHorsService()) || (!E->bChef && EnnemisEnEtat() == 0))
		{
			Nettoyer();
			EtapeSuivante();
		}
		break;

	case ESorteEtape::Proteger:
		if (VoitureTrajet)
		{
			AvancerTrajet(Trajet, VoitureTrajet, Dt);
			ActualiserEnnemis(Dt, VoitureTrajet->GetActorLocation(), VoitureTrajet);
			for (const FEnnemi& En : Ennemis)
			{
				if (En.Voiture && !En.Voiture->EstHorsService() && FVector::Dist2D(En.Voiture->GetActorLocation(), VoitureTrajet->GetActorLocation()) < 800.0)
				{
					VieAllieeBrute -= Dt * 7.0;
				}
			}
			VieAlliee = FMath::Clamp(VieAllieeBrute / FMath::Max(E->Vie, 1.0), 0.0, 1.0);
			if (VieAllieeBrute <= 0.0)
			{
				Echouer(RaisonEchec);
				return;
			}
			if (Trajet.Fini())
			{
				Nettoyer();
				EtapeSuivante();
			}
		}
		break;

	case ESorteEtape::Semer:
	{
		ActualiserEnnemis(Dt, J->GetPawn()->GetActorLocation(), J->GetPawn());
		double Plus = 1e9;
		bool bVu = false;
		for (const FEnnemi& En : Ennemis)
		{
			if (En.Voiture && !En.Voiture->EstHorsService())
			{
				Plus = FMath::Min(Plus, En.Pilote.Distance);
				bVu = bVu || En.Pilote.bVoitCible;
			}
		}
		TempsHorsVue = (Plus > 150.0 || !bVu) ? TempsHorsVue + Dt : 0.0;
		const bool bPointAtteint = !E->Point.IsZero() && FVector2D::Distance(Pos, E->Point) < E->Rayon;
		if (bPointAtteint || (E->Point.IsZero() && (TempsHorsVue > 8.0 || EnnemisEnEtat() == 0)))
		{
			Nettoyer();
			EtapeSuivante();
		}
		break;
	}

	case ESorteEtape::Course:
		for (int32 I = 0; I < Rivaux.Num(); I++)
		{
			AvancerTrajet(TrajetsRivaux[I], Rivaux[I], Dt);
			if (TrajetsRivaux[I].Fini())
			{
				Echouer(TEXT("un rival est arrivé avant toi"));
				return;
			}
		}
		if (E->Route.IsValidIndex(PointCourse) && FVector2D::Distance(Pos, E->Route[PointCourse]) < E->Rayon)
		{
			PointCourse++;
			if (PointCourse >= E->Route.Num())
			{
				Nettoyer();
				EtapeSuivante();
				return;
			}
			P->Cible = E->Route[PointCourse];
			P->PlacerBalise(E->Route[PointCourse], E->Rayon, FLinearColor(1.f, 0.72f, 0.2f));
		}
		break;

	case ESorteEtape::Infiltrer:
		ActualiserGardes(Dt);
		if (Detection >= 1.0)
		{
			Echouer(TEXT("tu t'es fait repérer"));
			return;
		}
		Invite = FVector2D::Distance(Pos, E->Point) < E->Rayon ? FString(TEXT("E : fouiller")) : FString();
		if (FVector2D::Distance(Pos, E->Point) < E->Rayon && J->DemandeAction() && !Mienne)
		{
			Nettoyer();
			EtapeSuivante();
		}
		break;

	case ESorteEtape::Attendre:
		if (TempsEtape >= FMath::Max(E->Temps, 0.1))
		{
			EtapeSuivante();
		}
		break;

	default:
		break;
	}
}

void APalmeraHistoire::TestAutomatique(double Dt)
{
	static double Temps = 0.0;
	static int32 ChapitreNote = -1;
	static int32 EtapeNotee = -1;
	static const FEtapeHistoire* Precedente = nullptr;
	static double TempsSurEtape = 0.0;
	APalmeraJoueur* J = Joueur();
	if (!J)
	{
		return;
	}
	Temps += Dt;
	const FEtapeHistoire* E = EtapeActuelle();
	if (Chapitre != ChapitreNote || IndexEtape != EtapeNotee)
	{
		ChapitreNote = Chapitre;
		EtapeNotee = IndexEtape;
		TempsSurEtape = 0.0;
		J->Noter(FString::Printf(TEXT("chapitre %d, etape %d, sorte %d : %s"), Chapitre, IndexEtape, E ? int32(E->Sorte) : -1, E ? *E->Consigne : TEXT("fin")));
	}
	TempsSurEtape += Dt;
	if (bTestPas)
	{
		return;
	}
	if (TempsSurEtape > 40.0 && E)
	{
		J->Noter(FString::Printf(TEXT("BLOQUE chapitre %d etape %d sorte %d"), Chapitre, IndexEtape, int32(E->Sorte)));
		EtapeSuivante();
		return;
	}
	if (ChoixOuvert())
	{
		Avancer();
		return;
	}
	if (Lignes.IsValidIndex(IndexLigne) && !bAuto)
	{
		TempsLigne = 1000.0;
		LigneSuivante();
		return;
	}
	if (Lignes.IsValidIndex(IndexLigne))
	{
		LigneSuivante();
		return;
	}
	SauterEtape(true);
}

void APalmeraHistoire::SauterEtape(bool bAgir)
{
	APalmeraJoueur* J = Joueur();
	const FEtapeHistoire* E = EtapeActuelle();
	if (!J || !E)
	{
		return;
	}
	const auto Aller = [&](const FVector2D& P)
	{
		if (APawn* Moi = J->GetPawn())
		{
			Moi->SetActorLocation(EnMetres(P.X, P.Y, 1.3), false, nullptr, ETeleportType::ResetPhysics);
		}
	};
	switch (E->Sorte)
	{
	case ESorteEtape::Aller:
		J->DescendrePourTest();
		Aller(E->Point);
		break;
	case ESorteEtape::Parler:
		if (APalmeraPNJ* Avec = PNJ(E->Pnj))
		{
			const FVector Q = Avec->GetActorLocation() / Metre;
			J->DescendrePourTest();
			Aller(FVector2D(Q.X + 2.0, Q.Y));
			if (bAgir)
			{
				J->ForcerAction();
			}
		}
		break;
	case ESorteEtape::Infiltrer:
		J->DescendrePourTest();
		Aller(E->Point);
		if (bAgir)
		{
			J->ForcerAction();
		}
		break;
	case ESorteEtape::Suivre:
	case ESorteEtape::Proteger:
		Trajet.S = Trajet.Longueur;
		break;
	case ESorteEtape::Detruire:
		for (FEnnemi& En : Ennemis)
		{
			if (En.Voiture)
			{
				En.Voiture->Abimer(100000.0);
			}
		}
		break;
	case ESorteEtape::Semer:
		if (!E->Point.IsZero())
		{
			Aller(E->Point);
		}
		for (FEnnemi& En : Ennemis)
		{
			if (En.Voiture)
			{
				En.Voiture->Abimer(100000.0);
			}
		}
		break;
	case ESorteEtape::Course:
		if (E->Route.IsValidIndex(PointCourse))
		{
			Aller(E->Route[PointCourse]);
		}
		break;
	default:
		break;
	}
}

void APalmeraHistoire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	if (bTestAuto || bTestPas)
	{
		TestAutomatique(Dt);
	}
	if (Lignes.IsValidIndex(IndexLigne))
	{
		TempsLigne += Dt;
		if (bAuto && TempsLigne > 2.2 + Lignes[IndexLigne].Texte.Len() * 0.058)
		{
			LigneSuivante();
		}
	}
	else if (AppelsEnAttente.Num() > 0 && !bChoixOuvert)
	{
		FinDesLignes();
	}
	if (!EstTerminee())
	{
		const FEtapeHistoire* E = EtapeActuelle();
		const bool bDialogueEtape = E && (E->Sorte == ESorteEtape::Scene || E->Sorte == ESorteEtape::Appel);
		if (!bDialogueEtape)
		{
			ActualiserEtape(Dt);
		}
	}
}
