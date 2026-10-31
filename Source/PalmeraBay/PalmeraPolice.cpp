#include "PalmeraPolice.h"

#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "PalmeraConfig.h"
#include "PalmeraPartie.h"
#include "PalmeraPilote.h"
#include "PalmeraVoiture.h"

using namespace Palmera;

namespace
{
	const int32 Effectifs[6] = { 0, 2, 3, 4, 6, 8 };
	const FVector Reserve(0, 0, -30000);
}

APalmeraPolice::APalmeraPolice()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	Hasard.Initialize(551);
}

void APalmeraPolice::BeginPlay()
{
	Super::BeginPlay();

	Etats.SetNum(8);
	for (int32 I = 0; I < 8; I++)
	{
		const FTransform Loin(Reserve + FVector(I * 600.0, 0, 0));
		APalmeraVoiture* V = GetWorld()->SpawnActorDeferred<APalmeraVoiture>(APalmeraVoiture::StaticClass(), Loin);
		FFicheVoiture F = APalmeraVoiture::FicheCivile(I % 3 == 2 ? 2 : 0);
		F.Peinture = FLinearColor(0.02f, 0.025f, 0.05f);
		F.Puissance *= 1.8;
		F.ForceMax *= 1.4;
		F.bPolice = true;
		V->Configurer(F);
		V->FinishSpawning(Loin);
		V->ActiverPhysique(false);
		V->SetActorHiddenInGame(true);
		V->SetActorEnableCollision(false);
		Patrouilles.Add(V);

		UPointLightComponent* G = NewObject<UPointLightComponent>(V);
		G->SetupAttachment(V->GetRootComponent());
		G->SetRelativeLocation(FVector(0, 0, 90));
		G->SetIntensityUnits(ELightUnits::Candelas);
		G->SetIntensity(0.0f);
		G->SetAttenuationRadius(2600.0f);
		G->SetCastShadows(false);
		G->RegisterComponent();
		Gyrophares.Add(G);
	}
}

void APalmeraPolice::Sortir(int32 Index)
{
	APalmeraPartie* Partie = APalmeraPartie::Lire(this);
	const APawn* Joueur = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (!Joueur)
	{
		return;
	}
	// Un carrefour éloigné, de préférence derrière le joueur
	const FVector Pos = Joueur->GetActorLocation() / Metre;
	const FVector Avant = Joueur->GetActorForwardVector();
	FVector2D Meilleur = FVector2D::ZeroVector;
	bool bTrouve = false;
	for (int32 Essai = 0; Essai < 30 && !bTrouve; Essai++)
	{
		const int32 Kx = FMath::Clamp(FMath::RoundToInt(Pos.X / Pas) + Hasard.RandRange(-3, 3), RueMin, RueMax);
		const int32 Ky = FMath::Clamp(FMath::RoundToInt(Pos.Y / Pas) + Hasard.RandRange(-3, 3), RueMin, RueMax);
		const FVector2D P(Kx * Pas, Ky * Pas + 30.0);
		const FVector2D Ecart = P - FVector2D(Pos.X, Pos.Y);
		const double D = Ecart.Size();
		if (D > 150.0 && D < 420.0 && FVector2D::DotProduct(Ecart / D, FVector2D(Avant.X, Avant.Y)) < 0.3)
		{
			Meilleur = P;
			bTrouve = true;
		}
	}
	if (!bTrouve)
	{
		return;
	}
	APalmeraVoiture* V = Patrouilles[Index];
	V->SetActorHiddenInGame(false);
	V->SetActorEnableCollision(true);
	V->SetActorLocationAndRotation(EnMetres(Meilleur.X - 1.75, Meilleur.Y, 1.2), FRotator(0.0, 90.0, 0.0), false, nullptr, ETeleportType::ResetPhysics);
	V->ActiverPhysique(true);
	V->AllumerPhares(true);
	Etats[Index] = FEtat();
	Etats[Index].bActive = true;
	(void)Partie;
}

void APalmeraPolice::Ranger(int32 Index)
{
	APalmeraVoiture* V = Patrouilles[Index];
	V->ActiverPhysique(false);
	V->SetActorEnableCollision(false);
	V->SetActorHiddenInGame(true);
	V->AllumerPhares(false);
	V->SetActorLocation(Reserve + FVector(Index * 600.0, 0, 0), false, nullptr, ETeleportType::ResetPhysics);
	Gyrophares[Index]->SetIntensity(0.0f);
	Etats[Index].bActive = false;
}

void APalmeraPolice::Piloter(int32 Index, const FVector& Cible, double Dt)
{
	const APawn* Joueur = GetWorld()->GetFirstPlayerController()->GetPawn();
	Palmera::PiloterVers(Patrouilles[Index], Cible, Joueur, Etats[Index].Pilote, Dt, 9.0);
	if (Etats[Index].Pilote.bVoitCible && Etats[Index].Pilote.Distance < 180.0)
	{
		bJoueurVu = true;
	}
	DistancePlusProche = FMath::Min(DistancePlusProche, Etats[Index].Pilote.Distance);
}

void APalmeraPolice::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	APalmeraPartie* Partie = APalmeraPartie::Lire(this);
	const APlayerController* Joueur = GetWorld()->GetFirstPlayerController();
	if (!Partie || !Joueur || !Joueur->GetPawn())
	{
		return;
	}
	const FVector Cible = Joueur->GetPawn()->GetActorLocation();
	const int32 Voulues = Effectifs[FMath::Clamp(Partie->Recherche, 0, 5)];

	bJoueurVu = false;
	DistancePlusProche = 1e9;
	int32 Actives = 0;
	const double Flash = FPlatformTime::Seconds() * 6.0;
	for (int32 I = 0; I < Patrouilles.Num(); I++)
	{
		if (!Etats[I].bActive)
		{
			continue;
		}
		Actives++;
		if (Actives > Voulues)
		{
			Ranger(I);
			continue;
		}
		Piloter(I, Cible, Dt);
		const bool bRouge = FMath::Fmod(Flash + I * 0.3, 1.0) < 0.5;
		Gyrophares[I]->SetLightColor(bRouge ? FLinearColor(1.f, 0.05f, 0.05f) : FLinearColor(0.1f, 0.2f, 1.f));
		Gyrophares[I]->SetIntensity(9000.0f);
		if (FVector::Dist2D(Patrouilles[I]->GetActorLocation(), Cible) > 75000.0)
		{
			Ranger(I);
		}
	}

	TempsJournal -= Dt;
	if (TempsJournal <= 0.0)
	{
		TempsJournal = 4.0;
		UE_LOG(LogTemp, Display, TEXT("PALMERA police : recherche=%d patrouilles=%d/%d plus proche=%.0f m vu=%d chaleur=%.0f"),
			Partie->Recherche, Actives, Voulues, DistancePlusProche, bJoueurVu ? 1 : 0, Partie->Chaleur);
	}

	TempsAppel -= Dt;
	if (Actives < Voulues && TempsAppel <= 0.0)
	{
		TempsAppel = 1.5;
		for (int32 I = 0; I < Patrouilles.Num(); I++)
		{
			if (!Etats[I].bActive)
			{
				Sortir(I);
				break;
			}
		}
	}
}
