#include "PalmeraPartie.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PalmeraConfig.h"
#include "PalmeraHeros.h"
#include "PalmeraHistoire.h"
#include "PalmeraJoueur.h"
#include "PalmeraPolice.h"
#include "PalmeraSauvegarde.h"
#include "PalmeraSons.h"
#include "PalmeraVille.h"
#include "PalmeraVoiture.h"

using namespace Palmera;

namespace
{
	const FLinearColor CouleurMission(1.f, 0.72f, 0.2f, 1.f);
	const FVector2D Hopital2D(540.0, -120.0);
	const FVector2D Commissariat(60.0, 240.0);
	const FVector2D Peinture(660.0, 240.0);
	const TCHAR* Emplacement = TEXT("Palmera");

	UStaticMeshComponent* CreerCylindre(AActor* Proprietaire)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Proprietaire);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		C->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiaux/M_Balise.M_Balise")));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(Proprietaire->GetRootComponent());
		C->RegisterComponent();
		return C;
	}
}

APalmeraPartie::APalmeraPartie()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Racine")));
}

APalmeraPartie* APalmeraPartie::Lire(const UObject* Contexte)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (!Monde)
	{
		return nullptr;
	}
	for (TActorIterator<APalmeraPartie> It(Monde); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

const TArray<FLieu>& APalmeraPartie::Lieux()
{
	static const TArray<FLieu> Liste = {
		{ TEXT("Garage Nina"), FVector2D(-660.0, 120.0), FLinearColor(0.2f, 0.8f, 1.f, 1.f) },
		{ TEXT("Commissariat"), Commissariat, FLinearColor(0.2f, 0.4f, 1.f, 1.f) },
		{ TEXT("Hôpital"), Hopital2D, FLinearColor(1.f, 0.25f, 0.3f, 1.f) },
		{ TEXT("Taxis Lune"), FVector2D(-300.0, 240.0), FLinearColor(1.f, 0.9f, 0.2f, 1.f) },
		{ TEXT("Peinture Express"), Peinture, FLinearColor(0.9f, 0.3f, 1.f, 1.f) },
		{ TEXT("Entrepôt 7"), FVector2D(660.0, -600.0), FLinearColor(0.8f, 0.6f, 0.4f, 1.f) },
		{ TEXT("Hôtel Océan"), FVector2D(-840.0, -240.0), FLinearColor(1.f, 0.6f, 0.5f, 1.f) }
	};
	return Liste;
}

FString APalmeraPartie::NomQuartier(const FVector2D& P)
{
	if (P.X < BordOuest - 11.0)
	{
		return TEXT("Plage de l'Ouest");
	}
	const int32 I = FMath::Clamp(FMath::FloorToInt(P.X / Pas), RueMin, RueMax - 1);
	const int32 J = FMath::Clamp(FMath::FloorToInt(P.Y / Pas), RueMin, RueMax - 1);
	switch (APalmeraVille::Quartier(I, J))
	{
	case EQuartier::Plage: return TEXT("Front de mer");
	case EQuartier::Parc: return TEXT("Parc des Palmiers");
	case EQuartier::Port: return TEXT("Port de Palmera");
	case EQuartier::Centre: return TEXT("Centre-ville");
	case EQuartier::Residentiel: return TEXT("Les Villas");
	default: return TEXT("Vieille ville");
	}
}

void APalmeraPartie::BeginPlay()
{
	Super::BeginPlay();

	DepartsMissions = { FVector2D(-960.0, -60.0), FVector2D(660.0, -600.0), Commissariat, FVector2D(-300.0, 240.0) };
	NomsMissions = { TEXT("Course du coucher de soleil"), TEXT("Livraison fragile"), TEXT("La grande évasion"), TEXT("Taxi de nuit") };

	FRandomStream H(77);
	for (int32 I = 0; I < 20; I++)
	{
		const int32 Ix = H.RandRange(RueMin, RueMax - 1);
		const int32 Iy = H.RandRange(RueMin, RueMax - 1);
		const double C = H.RandBool() ? 9.0 : -9.0;
		const FVector2D P = H.RandBool() ? FVector2D(Ix * Pas + C, (Iy + 0.3) * Pas) : FVector2D((Ix + 0.3) * Pas, Iy * Pas + C);
		Cassettes.Add(P);
		IdCassettes.Add(I);
	}

	if (const UPalmeraSauvegarde* S = Cast<UPalmeraSauvegarde>(UGameplayStatics::LoadGameFromSlot(Emplacement, 0)))
	{
		Argent = S->Argent;
		MissionsFinies = S->MissionsFinies;
		ChapitreSauve = S->Chapitre;
		DrapeauxSauves = S->Drapeaux;
		for (int32 I = Cassettes.Num() - 1; I >= 0; I--)
		{
			if (S->CassettesTrouvees & (1 << I))
			{
				bCassettesTrouvees[I] = true;
			}
		}
	}

	Balise = CreerCylindre(this);
	Balise->SetVisibility(false);
	CreerBalises();

	if (FParse::Value(FCommandLine::Get(), TEXT("-recherche="), Recherche))
	{
		Chaleur = Recherche * 20.0;
	}
}

void APalmeraPartie::CacherBalise()
{
	if (Balise)
	{
		Balise->SetVisibility(false);
	}
}

void APalmeraPartie::CreerBalises()
{
	for (const FVector2D& P : DepartsMissions)
	{
		UStaticMeshComponent* C = CreerCylindre(this);
		C->SetWorldLocation(EnMetres(P.X, P.Y, 8.0));
		C->SetWorldScale3D(FVector(0.09, 0.09, 0.16) * Metre);
		C->SetVectorParameterValueOnMaterials(TEXT("Couleur"), FVector(1.0, 0.65, 0.12));
		BalisesMissions.Add(C);
	}
	for (int32 I = 0; I < Cassettes.Num(); I++)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		C->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiaux/M_Neon.M_Neon")));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(RootComponent);
		C->RegisterComponent();
		C->SetWorldLocation(EnMetres(Cassettes[I].X, Cassettes[I].Y, 1.1));
		C->SetWorldScale3D(FVector(0.34, 0.03, 0.22));
		MaillesCassettes.Add(C);
	}
	for (int32 I = Cassettes.Num() - 1; I >= 0; I--)
	{
		if (bCassettesTrouvees[I])
		{
			MaillesCassettes[I]->DestroyComponent();
			MaillesCassettes.RemoveAt(I);
			Cassettes.RemoveAt(I);
			IdCassettes.RemoveAt(I);
			CassettesTrouvees++;
		}
	}
}

void APalmeraPartie::Annoncer(const FString& Texte, double Duree)
{
	Message = Texte;
	FinMessage = GetWorld()->GetTimeSeconds() + Duree;
}

void APalmeraPartie::Gagner(int32 Montant)
{
	Argent += Montant;
}

void APalmeraPartie::Sauvegarder()
{
	UPalmeraSauvegarde* S = Cast<UPalmeraSauvegarde>(UGameplayStatics::CreateSaveGameObject(UPalmeraSauvegarde::StaticClass()));
	S->Argent = Argent;
	S->MissionsFinies = MissionsFinies;
	S->Chapitre = ChapitreSauve;
	S->Drapeaux = DrapeauxSauves;
	int32 Masque = 0;
	for (int32 I = 0; I < 32; I++)
	{
		if (bCassettesTrouvees[I])
		{
			Masque |= 1 << I;
		}
	}
	S->CassettesTrouvees = Masque;
	UGameplayStatics::SaveGameToSlot(S, Emplacement, 0);
}

void APalmeraPartie::Faute(double Gravite)
{
	Chaleur = FMath::Min(Chaleur + Gravite, 100.0);
	Recherche = FMath::Max(Recherche, FMath::Min(FMath::FloorToInt(Chaleur / 20.0), 5));
}

void APalmeraPartie::PlacerBalise(const FVector2D& Position, double Rayon, const FLinearColor& Couleur)
{
	Balise->SetVisibility(true);
	Balise->SetWorldLocation(EnMetres(Position.X, Position.Y, 12.0));
	Balise->SetWorldScale3D(FVector(Rayon * 2.0, Rayon * 2.0, 24.0));
	Balise->SetVectorParameterValueOnMaterials(TEXT("Couleur"), FVector(Couleur.R, Couleur.G, Couleur.B));
}

void APalmeraPartie::DemarrerMission(int32 Index)
{
	Etapes.Reset();
	MissionActuelle = Index;
	switch (Index)
	{
	case -1:
		Recompense = 250;
		Etapes.Add({ FVector2D(-969.0, 44.0), 0.0, -1.0, TEXT("Monte dans la voiture rouge (touche F)"), 1 });
		Etapes.Add({ FVector2D(-660.0, 120.0), 11.0, -1.0, TEXT("Rejoins le Garage Nina (point jaune sur la carte)"), 0 });
		break;
	case 0:
		Recompense = 900;
		Etapes.Add({ FVector2D(-960.0, -480.0), 12.0, 40.0, TEXT("Course : suis les points de passage"), 0 });
		Etapes.Add({ FVector2D(-480.0, -480.0), 12.0, 20.0, TEXT("Course : point 2 sur 6"), 0 });
		Etapes.Add({ FVector2D(-480.0, 480.0), 12.0, 34.0, TEXT("Course : point 3 sur 6"), 0 });
		Etapes.Add({ FVector2D(0.0, 480.0), 12.0, 20.0, TEXT("Course : point 4 sur 6"), 0 });
		Etapes.Add({ FVector2D(0.0, -480.0), 12.0, 34.0, TEXT("Course : point 5 sur 6"), 0 });
		Etapes.Add({ FVector2D(-960.0, -480.0), 12.0, 34.0, TEXT("Course : dernier point !"), 0 });
		break;
	case 1:
		Recompense = 1200;
		Etapes.Add({ FVector2D(660.0, -600.0), 12.0, -1.0, TEXT("Prends une voiture pour charger la caisse"), 1 });
		Etapes.Add({ FVector2D(-840.0, -240.0), 12.0, 95.0, TEXT("Livre la caisse à l'Hôtel Océan sans aucun choc"), 3 });
		break;
	case 2:
		Recompense = 1500;
		Etapes.Add({ Peinture, 12.0, 130.0, TEXT("Sème la police : file à Peinture Express"), 2 });
		break;
	default:
		Recompense = 1100;
		Etapes.Add({ FVector2D(-120.0, -240.0), 9.0, 70.0, TEXT("Taxi : va chercher le client"), 0 });
		Etapes.Add({ FVector2D(540.0, -120.0), 9.0, 60.0, TEXT("Dépose le client à l'hôpital"), 0 });
		Etapes.Add({ FVector2D(-600.0, -480.0), 9.0, 75.0, TEXT("Nouveau client au Vieux Port"), 0 });
		Etapes.Add({ FVector2D(240.0, 600.0), 9.0, 80.0, TEXT("Dépose le client aux Villas"), 0 });
		break;
	}
	bEnMission = true;
	EtapeActuelle = -1;
	for (UStaticMeshComponent* B : BalisesMissions)
	{
		B->SetVisibility(false);
	}
	EtapeSuivante();
}

void APalmeraPartie::EtapeSuivante()
{
	EtapeActuelle++;
	if (EtapeActuelle >= Etapes.Num())
	{
		Reussir(Recompense);
		return;
	}
	const FEtape& E = Etapes[EtapeActuelle];
	Objectif = E.Consigne;
	bCibleActive = E.Sorte != 1;
	Cible = E.Point;
	Chrono = E.Temps;
	if (E.Sorte == 2)
	{
		Chaleur = FMath::Max(Chaleur, 60.0);
		Recherche = FMath::Max(Recherche, 3);
	}
	if (bCibleActive)
	{
		PlacerBalise(E.Point, E.Rayon, CouleurMission);
	}
	else
	{
		Balise->SetVisibility(false);
	}
}

void APalmeraPartie::Reussir(int32 Somme)
{
	Gagner(Somme);
	Annoncer(FString::Printf(TEXT("MISSION RÉUSSIE  +%d $"), Somme), 5.0);
	if (APalmeraSons* Sons = APalmeraSons::Lire(this))
	{
		Sons->Jouer(TEXT("S_Mission"), 0.8f);
	}
	MissionsFinies = FMath::Max(MissionsFinies, 1) + (MissionActuelle >= 0 ? 1 : 0);
	bEnMission = false;
	bCibleActive = false;
	MissionActuelle = -1;
	Objectif.Empty();
	Chrono = -1.0;
	Balise->SetVisibility(false);
	for (UStaticMeshComponent* B : BalisesMissions)
	{
		B->SetVisibility(true);
	}
	if (Recherche > 0 && Etapes.Num() == 1)
	{
		Recherche = 0;
		Chaleur = 0.0;
	}
	Sauvegarder();
}

void APalmeraPartie::Echouer(const FString& Raison)
{
	Annoncer(FString::Printf(TEXT("MISSION RATÉE : %s"), *Raison), 5.0);
	bEnMission = false;
	bCibleActive = false;
	MissionActuelle = -1;
	Objectif.Empty();
	Chrono = -1.0;
	Balise->SetVisibility(false);
	for (UStaticMeshComponent* B : BalisesMissions)
	{
		B->SetVisibility(true);
	}
}

void APalmeraPartie::Arrete()
{
	const int32 Amende = FMath::Min(Argent, 200 + Recherche * 100);
	Argent -= Amende;
	Recherche = 0;
	Chaleur = 0.0;
	if (bEnMission)
	{
		Echouer(TEXT("tu t'es fait arrêter"));
	}
	Annoncer(FString::Printf(TEXT("ARRÊTÉ·E  -%d $"), Amende), 5.0);
	if (APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController()))
	{
		J->Ramener(EnMetres(Commissariat.X, Commissariat.Y + 8.0, 1.5));
	}
}

void APalmeraPartie::Hopital()
{
	const int32 Frais = FMath::Min(Argent, 150);
	Argent -= Frais;
	Recherche = 0;
	Chaleur = 0.0;
	if (bEnMission)
	{
		Echouer(TEXT("tu es tombé·e à l'eau"));
	}
	Annoncer(FString::Printf(TEXT("REPÊCHÉ·E  -%d $"), Frais), 5.0);
	if (APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController()))
	{
		J->Ramener(EnMetres(Hopital2D.X, Hopital2D.Y + 8.0, 1.5));
	}
}

void APalmeraPartie::ActualiserCassettes()
{
	const APlayerController* J = GetWorld()->GetFirstPlayerController();
	const APawn* Moi = J ? J->GetPawn() : nullptr;
	if (!Moi)
	{
		return;
	}
	const FVector P = Moi->GetActorLocation() / Metre;
	for (int32 I = Cassettes.Num() - 1; I >= 0; I--)
	{
		MaillesCassettes[I]->AddWorldRotation(FRotator(0.0, 90.0 * GetWorld()->GetDeltaSeconds(), 0.0));
		if (FVector2D::DistSquared(Cassettes[I], FVector2D(P.X, P.Y)) < 9.0)
		{
			bCassettesTrouvees[IdCassettes[I]] = true;
			MaillesCassettes[I]->DestroyComponent();
			MaillesCassettes.RemoveAt(I);
			Cassettes.RemoveAt(I);
			IdCassettes.RemoveAt(I);
			CassettesTrouvees++;
			Gagner(100);
			Annoncer(FString::Printf(TEXT("Cassette trouvée : %d / 20  +100 $"), CassettesTrouvees), 3.0);
			if (APalmeraHistoire* Recit = APalmeraHistoire::Lire(this))
			{
				Recit->JouerCassette(CassettesTrouvees - 1);
			}
			if (APalmeraSons* Sons = APalmeraSons::Lire(this))
			{
				Sons->Jouer(TEXT("S_Cassette"), 0.7f);
			}
			Sauvegarder();
		}
	}
}

void APalmeraPartie::ActualiserMission(double Dt)
{
	APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	const APawn* Moi = J ? J->GetPawn() : nullptr;
	if (!Moi)
	{
		return;
	}
	const FVector P3 = Moi->GetActorLocation() / Metre;
	const FVector2D P(P3.X, P3.Y);

	// Tombé à l'eau : hors de la ville, sous le niveau de la mer
	if (P3.Z < NiveauMer - 0.6)
	{
		Hopital();
		return;
	}

	// Le garage répare gratuitement, Peinture Express efface la recherche
	Aide.Empty();
	if (APalmeraVoiture* Mienne = J->VoitureActuelle())
	{
		if (FVector2D::Distance(P, Lieux()[0].Position) < 16.0 && Mienne->LireVie() < Mienne->LireVieMax())
		{
			Aide = TEXT("E : réparer la voiture");
			if (J->DemandeAction())
			{
				Mienne->Reparer();
				Annoncer(TEXT("Voiture réparée"), 3.0);
			}
		}
		else if (FVector2D::Distance(P, Peinture) < 16.0 && Recherche > 0 && Argent >= 300)
		{
			Aide = TEXT("E : repeindre la voiture (300 $)");
			if (J->DemandeAction())
			{
				Argent -= 300;
				Recherche = 0;
				Chaleur = 0.0;
				Mienne->Reparer();
				Annoncer(TEXT("Nouvelle couleur : la police ne te reconnaît plus"), 4.0);
			}
		}
	}

	// Arrestation : la police est tout près et le joueur ne bouge presque plus
	for (TActorIterator<APalmeraPolice> It(GetWorld()); It; ++It)
	{
		if (Recherche > 0)
		{
			const double Vitesse = Moi->GetVelocity().Size() / Metre;
			TempsArrestation = (It->DistancePlusProche < 8.0 && Vitesse < 3.0) ? TempsArrestation + Dt : 0.0;
			if (TempsArrestation > 3.0)
			{
				TempsArrestation = 0.0;
				Arrete();
				return;
			}
			if (!It->bJoueurVu)
			{
				Chaleur = FMath::Max(Chaleur - Dt * 3.5, 0.0);
				const int32 Niveau = FMath::CeilToInt(Chaleur / 20.0);
				if (Niveau < Recherche)
				{
					Recherche = Niveau;
					if (Recherche == 0)
					{
						Annoncer(TEXT("Police semée !"), 3.0);
					}
				}
			}
		}
	}

	const APalmeraHistoire* Recit = APalmeraHistoire::Lire(this);
	if (Recit && Recit->EstOccupee())
	{
		return;
	}
	if (!bEnMission && Recit && Recit->ChapitreActuel() == 0)
	{
		return;
	}
	if (!bEnMission)
	{
		if (!Recit || Recit->EstTerminee())
		{
			Objectif.Empty();
		}
		for (int32 I = 0; I < DepartsMissions.Num(); I++)
		{
			if (FVector2D::DistSquared(P, DepartsMissions[I]) < FMath::Square(10.0))
			{
				Objectif = FString::Printf(TEXT("E : lancer « %s »"), *NomsMissions[I]);
				if (J->DemandeAction())
				{
					DemarrerMission(I);
				}
				break;
			}
		}
		return;
	}

	if (Chrono >= 0.0)
	{
		Chrono -= Dt;
		if (Chrono <= 0.0)
		{
			Echouer(TEXT("temps écoulé"));
			return;
		}
	}
	if (!Etapes.IsValidIndex(EtapeActuelle))
	{
		return;
	}
	const FEtape& E = Etapes[EtapeActuelle];
	if (E.Sorte == 1)
	{
		if (J->VoitureActuelle())
		{
			EtapeSuivante();
		}
		return;
	}
	if (E.Sorte == 3 && J->VoitureActuelle())
	{
		// Caisse fragile : un freinage brutal contre un obstacle casse tout
		if (J->VoitureActuelle()->ChocRecent() > 8.0)
		{
			Echouer(TEXT("la caisse est cassée"));
			return;
		}
	}
	if (FVector2D::DistSquared(P, E.Point) < E.Rayon * E.Rayon)
	{
		EtapeSuivante();
	}
}

void APalmeraPartie::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ActualiserMission(DeltaSeconds);
	ActualiserCassettes();
}
