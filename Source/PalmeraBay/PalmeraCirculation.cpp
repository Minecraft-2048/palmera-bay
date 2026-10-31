#include "PalmeraCirculation.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PalmeraConfig.h"
#include "PalmeraMonde.h"
#include "PalmeraVoiture.h"

using namespace Palmera;

namespace
{
	const FLinearColor Couleurs[] = {
		FLinearColor(0.72f, 0.72f, 0.74f), FLinearColor(0.02f, 0.02f, 0.025f), FLinearColor(0.85f, 0.85f, 0.83f),
		FLinearColor(0.08f, 0.1f, 0.28f), FLinearColor(0.42f, 0.03f, 0.03f), FLinearColor(0.2f, 0.22f, 0.24f),
		FLinearColor(0.8f, 0.45f, 0.04f), FLinearColor(0.05f, 0.25f, 0.2f), FLinearColor(0.75f, 0.68f, 0.5f)
	};

	FVector2D Bezier(const FVector2D& A, const FVector2D& B, const FVector2D& C, double T)
	{
		const double U = 1.0 - T;
		return A * (U * U) + B * (2.0 * U * T) + C * (T * T);
	}
}

APalmeraCirculation::APalmeraCirculation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	Hasard.Initialize(9173);
}

bool APalmeraCirculation::FeuVert(int32 Axe, double T)
{
	const double Phase = FMath::Fmod(T + (Axe == 0 ? 12.0 : 0.0), 24.0);
	return Phase < 10.0;
}

FVector2D APalmeraCirculation::PointVoie(int32 Axe, int32 Rue, int32 Sens, int32 Voie, double S) const
{
	const double Decalage = Voie == 0 ? 1.75 : 5.25;
	return Axe == 0 ? FVector2D(Rue * Pas - Sens * Decalage, S) : FVector2D(S, Rue * Pas + Sens * Decalage);
}

void APalmeraCirculation::BeginPlay()
{
	Super::BeginPlay();

	FParse::Value(FCommandLine::Get(), TEXT("-voitures="), NbVoitures);
	Conducteurs.SetNum(NbVoitures);
	for (int32 I = 0; I < NbVoitures; I++)
	{
		const FTransform Loin(FVector(0, 0, -5000.0 - I * 400.0));
		APalmeraVoiture* V = GetWorld()->SpawnActorDeferred<APalmeraVoiture>(APalmeraVoiture::StaticClass(), Loin);
		static const int32 Modeles[] = { 0, 1, 2, 0, 3, 1, 5, 0, 4, 1, 2, 0, 5, 3 };
		const int32 Modele = Modeles[I % UE_ARRAY_COUNT(Modeles)];
		FFicheVoiture F = (I % 23 == 22) ? APalmeraVoiture::FicheSport() : APalmeraVoiture::FicheCivile(Modele);
		F.Peinture = Modele == 5 ? FLinearColor(0.85f, 0.56f, 0.02f) : Couleurs[(I * 5) % UE_ARRAY_COUNT(Couleurs)];
		V->Configurer(F);
		V->FinishSpawning(Loin);
		V->ActiverPhysique(false);
		V->OnActorHit.AddDynamic(this, &APalmeraCirculation::SurChoc);
		Voitures.Add(V);
		Placer(I, true);
	}
}

bool APalmeraCirculation::TrouverPlace(FConducteur& C, const FVector& Autour, double RayonMin, double RayonMax, const FVector& Regard)
{
	const FVector2D Centre(Autour.X / Metre, Autour.Y / Metre);
	for (int32 Essai = 0; Essai < 40; Essai++)
	{
		C.Axe = Hasard.RandRange(0, 1);
		C.Sens = Hasard.RandBool() ? 1 : -1;
		C.Voie = Hasard.RandRange(0, 1);
		const double Base = C.Axe == 0 ? Centre.X : Centre.Y;
		const int32 Ecart = FMath::RoundToInt(RayonMax / Pas);
		C.Rue = FMath::Clamp(FMath::RoundToInt(Base / Pas) + Hasard.RandRange(-Ecart, Ecart), RueMin, RueMax);
		const double Long = C.Axe == 0 ? Centre.Y : Centre.X;
		C.S = FMath::Clamp(Long + Hasard.FRandRange(-RayonMax, RayonMax), BordNord + 20.0, BordSud - 20.0);

		// Loin des carrefours pour ne pas apparaître au milieu d'un croisement
		const double Reste = FMath::Abs(FMath::Fmod(C.S, Pas));
		if (Reste < 22.0 || Reste > Pas - 22.0)
		{
			continue;
		}
		const FVector2D P = PointVoie(C.Axe, C.Rue, C.Sens, C.Voie, C.S);
		const double D = FVector2D::Distance(P, Centre);
		if (D < RayonMin || D > RayonMax)
		{
			continue;
		}
		const FVector Vers = (FVector(P.X, P.Y, 0.0) - FVector(Centre.X, Centre.Y, 0.0)).GetSafeNormal();
		if (!Regard.IsZero() && D < 300.0 && FVector::DotProduct(Vers, Regard) > 0.1)
		{
			continue;	// dans le champ de vision : on cherche ailleurs
		}
		bool bLibre = true;
		for (const APalmeraVoiture* Autre : Voitures)
		{
			if (FVector::DistSquared2D(Autre->GetActorLocation(), EnMetres(P.X, P.Y)) < FMath::Square(1400.0))
			{
				bLibre = false;
				break;
			}
		}
		if (bLibre)
		{
			return true;
		}
	}
	return false;
}

void APalmeraCirculation::Placer(int32 Index, bool bPremierTour)
{
	FConducteur& C = Conducteurs[Index];
	APalmeraVoiture* V = Voitures[Index];
	APlayerController* Joueur = GetWorld()->GetFirstPlayerController();
	FVector Autour = EnMetres(BordOuest - 13.5, 32.0);
	FVector Regard = FVector::ZeroVector;
	if (Joueur && Joueur->PlayerCameraManager && !bPremierTour)
	{
		Autour = Joueur->PlayerCameraManager->GetCameraLocation();
		Regard = Joueur->PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal2D();
	}
	if (!TrouverPlace(C, Autour, bPremierTour ? 30.0 : 170.0, 430.0, Regard))
	{
		return;
	}
	C.bVirage = false;
	C.bAccidente = false;
	C.TempsAccident = 0.0;
	C.VitesseVoulue = Hasard.FRandRange(10.0f, 14.5f);
	C.Vitesse = C.VitesseVoulue;
	V->ActiverPhysique(false);
	const FVector2D P = PointVoie(C.Axe, C.Rue, C.Sens, C.Voie, C.S);
	const double Lacet = C.Axe == 0 ? (C.Sens > 0 ? 90.0 : -90.0) : (C.Sens > 0 ? 0.0 : 180.0);
	V->SetActorLocationAndRotation(EnMetres(P.X, P.Y) + FVector(0, 0, V->LireFiche().CentreBoite.Z), FRotator(0.0, Lacet, 0.0), false, nullptr, ETeleportType::ResetPhysics);
}

void APalmeraCirculation::PreparerCarrefour(FConducteur& C, int32 Carrefour)
{
	// Tout droit le plus souvent ; une voie de droite peut tourner à droite, une voie de gauche à gauche
	int32 Choix = 0;	// 0 : tout droit, 1 : droite, -1 : gauche
	const double Tirage = Hasard.FRand();
	if (C.Voie == 1 && Tirage < 0.35)
	{
		Choix = 1;
	}
	else if (C.Voie == 0 && Tirage < 0.22)
	{
		Choix = -1;
	}
	const int32 Suivant = Carrefour + C.Sens;
	if (Choix == 0 && (Suivant < RueMin || Suivant > RueMax))
	{
		Choix = C.Voie == 1 ? 1 : -1;	// la rue s'arrête ici : il faut tourner
	}

	const int32 SensDroite = C.Axe == 0 ? -C.Sens : C.Sens;
	const int32 SensApres = Choix == 1 ? SensDroite : -SensDroite;
	if (Choix != 0 && (C.Rue + SensApres < RueMin || C.Rue + SensApres > RueMax))
	{
		Choix = 0;
	}
	if (Choix == 0)
	{
		return;
	}

	C.bVirage = true;
	C.T = 0.0;
	C.AxeApres = 1 - C.Axe;
	C.RueApres = Carrefour;
	C.SensApres = SensApres;
	C.VoieApres = C.Voie;
	C.SApres = C.Rue * Pas + SensApres * 10.5;
	C.P0 = PointVoie(C.Axe, C.Rue, C.Sens, C.Voie, Carrefour * Pas - C.Sens * 10.5);
	C.P2 = PointVoie(C.AxeApres, C.RueApres, C.SensApres, C.VoieApres, C.SApres);
	C.P1 = C.Axe == 0 ? FVector2D(C.P0.X, C.P2.Y) : FVector2D(C.P2.X, C.P0.Y);
	C.LongueurVirage = FMath::Max(FVector2D::Distance(C.P0, C.P1) + FVector2D::Distance(C.P1, C.P2), 4.0) * 0.8;
}

double APalmeraCirculation::DistanceDevant(int32 Index, const FVector& Position, const FVector& Direction) const
{
	double Meilleur = 1000.0;
	const auto Tester = [&](const FVector& Autre)
	{
		const FVector Ecart = (Autre - Position) / Metre;
		const double Devant = FVector::DotProduct(FVector(Ecart.X, Ecart.Y, 0.0), Direction);
		if (Devant < 0.5 || Devant > 45.0)
		{
			return;
		}
		const double Lateral = FMath::Abs(FVector::DotProduct(FVector(Ecart.X, Ecart.Y, 0.0), FVector(-Direction.Y, Direction.X, 0.0)));
		if (Lateral < 1.7)
		{
			Meilleur = FMath::Min(Meilleur, Devant);
		}
	};
	for (int32 I = 0; I < Voitures.Num(); I++)
	{
		if (I != Index)
		{
			Tester(Voitures[I]->GetActorLocation());
		}
	}
	for (const APalmeraVoiture* Autre : Autres)
	{
		Tester(Autre->GetActorLocation());
	}
	if (const APlayerController* Joueur = GetWorld()->GetFirstPlayerController())
	{
		if (const APawn* P = Joueur->GetPawn())
		{
			Tester(P->GetActorLocation());
		}
	}
	return Meilleur;
}

void APalmeraCirculation::Avancer(FConducteur& C, APalmeraVoiture* V, double DeltaS, double Dt)
{
	FVector2D P;
	double Lacet;
	if (C.bVirage)
	{
		C.T = FMath::Min(C.T + DeltaS / C.LongueurVirage, 1.0);
		P = Bezier(C.P0, C.P1, C.P2, C.T);
		const FVector2D Tangente = (C.P1 - C.P0) * (1.0 - C.T) + (C.P2 - C.P1) * C.T;
		Lacet = FMath::RadiansToDegrees(FMath::Atan2(Tangente.Y, Tangente.X));
		if (C.T >= 1.0)
		{
			C.bVirage = false;
			C.Axe = C.AxeApres;
			C.Rue = C.RueApres;
			C.Sens = C.SensApres;
			C.Voie = C.VoieApres;
			C.S = C.SApres;
		}
	}
	else
	{
		const double Avant = C.S;
		C.S += C.Sens * DeltaS;
		// Franchit-on la ligne de décision d'un carrefour ?
		const int32 Carrefour = FMath::RoundToInt(C.S / Pas);
		const double Ligne = Carrefour * Pas - C.Sens * 10.5;
		if ((Avant - Ligne) * C.Sens < 0.0 && (C.S - Ligne) * C.Sens >= 0.0)
		{
			PreparerCarrefour(C, Carrefour);
		}
		P = PointVoie(C.Axe, C.Rue, C.Sens, C.Voie, C.S);
		Lacet = C.Axe == 0 ? (C.Sens > 0 ? 90.0 : -90.0) : (C.Sens > 0 ? 0.0 : 180.0);
	}
	V->SetActorLocationAndRotation(EnMetres(P.X, P.Y) + FVector(0, 0, V->LireFiche().CentreBoite.Z), FRotator(0.0, Lacet, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	V->AnimerRoues(C.Vitesse, Dt);
}

void APalmeraCirculation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	TempsFeux = FMath::Fmod(TempsFeux + Dt, 24.0);

	APlayerController* Joueur = GetWorld()->GetFirstPlayerController();
	const FVector Camera = (Joueur && Joueur->PlayerCameraManager) ? Joueur->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;

	bool bNuit = false;
	for (TActorIterator<APalmeraMonde> It(GetWorld()); It; ++It)
	{
		bNuit = It->Nuit > 0.45;
	}
	Autres.Reset();
	for (TActorIterator<APalmeraVoiture> It(GetWorld()); It; ++It)
	{
		if (!Voitures.Contains(*It) && !It->IsHidden())
		{
			Autres.Add(*It);
		}
	}

	for (int32 I = 0; I < Conducteurs.Num(); I++)
	{
		FConducteur& C = Conducteurs[I];
		APalmeraVoiture* V = Voitures[I];
		V->AllumerLueurs(bNuit);

		const double Distance = FVector::Dist2D(V->GetActorLocation(), Camera) / Metre;

		if (V->bCibleJoueur)
		{
			C.bAccidente = true;
			C.TempsAccident = 0.0;
			continue;
		}
		if (C.bAccidente)
		{
			C.TempsAccident += Dt;
			if (Distance > 200.0 || (C.TempsAccident > 40.0 && Distance > 120.0))
			{
				Placer(I, false);
			}
			continue;
		}
		if (Distance > 470.0)
		{
			Placer(I, false);
			continue;
		}

		const FVector Direction = V->GetActorForwardVector().GetSafeNormal2D();
		const double Devant = DistanceDevant(I, V->GetActorLocation(), Direction);
		double Cible = C.VitesseVoulue;
		if (C.bVirage)
		{
			Cible = FMath::Min(Cible, 7.5);
		}
		if (Devant < 44.0)
		{
			// Garder au moins 8 m à l'arrêt, plus une seconde et demie de distance en roulant
			Cible = FMath::Min(Cible, FMath::Max(Devant - 8.0, 0.0) / 1.5);
		}
		if (!C.bVirage)
		{
			const int32 Carrefour = FMath::RoundToInt(C.S / Pas) + (((C.S - FMath::RoundToInt(C.S / Pas) * Pas) * C.Sens > -13.0) ? C.Sens : 0);
			const double Ligne = Carrefour * Pas - C.Sens * 13.0;
			const double Reste = (Ligne - C.S) * C.Sens;
			if (Reste >= -1.5 && Reste < 42.0 && !FeuVert(C.Axe, TempsFeux))
			{
				Cible = FMath::Min(Cible, FMath::Sqrt(2.0 * 3.5 * FMath::Max(Reste, 0.0)));
			}
		}
		const double Acceleration = Cible > C.Vitesse ? 2.8 : 6.5;
		V->AllumerStop(Cible < C.Vitesse - 0.4 || C.Vitesse < 0.3);
		C.Vitesse = FMath::FInterpConstantTo(C.Vitesse, Cible, Dt, Acceleration);
		Avancer(C, V, C.Vitesse * Dt, Dt);
	}

	if (UMaterialParameterCollection* Param = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiaux/MPC_Palmera.MPC_Palmera")))
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, Param, TEXT("TempsFeux"), float(TempsFeux));
	}
}

void APalmeraCirculation::SurChoc(AActor* Soi, AActor* Autre, FVector Impulsion, const FHitResult& Touche)
{
	const int32 Index = Voitures.IndexOfByKey(Cast<APalmeraVoiture>(Soi));
	if (Index == INDEX_NONE || Conducteurs[Index].bAccidente)
	{
		return;
	}
	// Seul un choc avec une voiture lancée met fin à la conduite automatique
	const APalmeraVoiture* Heurte = Cast<APalmeraVoiture>(Autre);
	if (!Heurte || !Heurte->PhysiqueActive() || Heurte->GetVelocity().Size() < 250.0)
	{
		return;
	}
	Conducteurs[Index].bAccidente = true;
	Voitures[Index]->ActiverPhysique(true);
	Voitures[Index]->Commander(0.0, 0.0, true);
}
