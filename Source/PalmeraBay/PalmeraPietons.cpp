#include "PalmeraPietons.h"

#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PalmeraConfig.h"
#include "PalmeraPartie.h"
#include "PalmeraPNJ.h"
#include "PalmeraVoiture.h"

using namespace Palmera;

namespace
{
	// Demi-côté du tour d'un bloc, au milieu du trottoir
	const double DemiTour = (Pas - 18.0) / 2.0;
	const double LongueurTour = DemiTour * 8.0;

	const FLinearColor Hauts[] = {
		FLinearColor(0.75f, 0.2f, 0.15f), FLinearColor(0.1f, 0.3f, 0.6f), FLinearColor(0.85f, 0.8f, 0.7f), FLinearColor(0.1f, 0.5f, 0.4f),
		FLinearColor(0.9f, 0.55f, 0.1f), FLinearColor(0.5f, 0.2f, 0.55f), FLinearColor(0.15f, 0.15f, 0.17f), FLinearColor(0.95f, 0.45f, 0.5f)
	};
}

APalmeraPassant::APalmeraPassant()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 90.0f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	GetMesh()->bVisibleInRayTracing = false;	// recalculer 26 corps par image pour le lancer de rayons coûte cher et ne se voit pas
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->bOrientRotationToMovement = true;
	M->RotationRate = FRotator(0, 360, 0);
	M->MaxWalkSpeed = 170.0f;
	M->bRunPhysicsWithNoController = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void APalmeraPassant::Habiller(FRandomStream& H)
{
	static const struct { const TCHAR* Nom; bool bRecolorer; } Modeles[] = {
		{ TEXT("Passant1"), true }, { TEXT("Passant2"), false }, { TEXT("Passant3"), false }, { TEXT("Passant4"), true },
		{ TEXT("Passante1"), true }, { TEXT("Passante2"), false }, { TEXT("Passante3"), false }, { TEXT("Passante4"), false }
	};
	const int32 Rang = H.RandRange(0, UE_ARRAY_COUNT(Modeles) - 1);
	FFichePNJ F;
	F.Modele = Modeles[Rang].Nom;
	F.bRecolorer = Modeles[Rang].bRecolorer;
	F.Haut = Hauts[H.RandRange(0, UE_ARRAY_COUNT(Hauts) - 1)];
	Palmera::Habiller(GetMesh(), F);
}

void APalmeraPassant::Marcher(const FVector& Direction, float Vitesse)
{
	GetCharacterMovement()->MaxWalkSpeed = Vitesse;
	GetCharacterMovement()->AddInputVector(Direction);
}

APalmeraPietons::APalmeraPietons()
{
	PrimaryActorTick.bCanEverTick = true;
	Hasard.Initialize(3141);
}

FVector2D APalmeraPietons::PointTour(const FMarche& M, double S) const
{
	const double D = DemiTour;
	double T = FMath::Fmod(S, LongueurTour);
	if (T < 0.0)
	{
		T += LongueurTour;
	}
	if (T < 2.0 * D)
	{
		return M.Centre + FVector2D(-D + T, -D);
	}
	if (T < 4.0 * D)
	{
		return M.Centre + FVector2D(D, -D + (T - 2.0 * D));
	}
	if (T < 6.0 * D)
	{
		return M.Centre + FVector2D(D - (T - 4.0 * D), D);
	}
	return M.Centre + FVector2D(-D, D - (T - 6.0 * D));
}

void APalmeraPietons::Placer(int32 Index, const FVector& Autour, bool bPremierTour)
{
	FMarche& M = Marches[Index];
	const FVector A = Autour / Metre;
	const int32 Ix = FMath::Clamp(FMath::FloorToInt(A.X / Pas) + Hasard.RandRange(-2, 2), RueMin, RueMax - 1);
	const int32 Iy = FMath::Clamp(FMath::FloorToInt(A.Y / Pas) + Hasard.RandRange(-2, 2), RueMin, RueMax - 1);
	M.Centre = FVector2D((Ix + 0.5) * Pas, (Iy + 0.5) * Pas);
	M.Position = Hasard.FRandRange(0.f, float(LongueurTour));
	M.Sens = Hasard.RandBool() ? 1 : -1;
	M.Vitesse = Hasard.FRandRange(1.1f, 1.7f);
	M.Peur = 0.0;
	const FVector2D P = PointTour(M, M.Position);
	Passants[Index]->SetActorLocation(EnMetres(P.X, P.Y, 1.1), false, nullptr, ETeleportType::ResetPhysics);
}

void APalmeraPietons::BeginPlay()
{
	Super::BeginPlay();

	Marches.SetNum(NbPassants);
	const FVector Depart = EnMetres(BordOuest + 40.0, 32.0);
	for (int32 I = 0; I < NbPassants; I++)
	{
		FActorSpawnParameters Regles;
		Regles.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APalmeraPassant* P = GetWorld()->SpawnActor<APalmeraPassant>(EnMetres(0, 0, 200 + I * 3.0), FRotator::ZeroRotator, Regles);
		P->Habiller(Hasard);
		Passants.Add(P);
		Placer(I, Depart, true);
	}
}

void APalmeraPietons::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	const APlayerController* Joueur = GetWorld()->GetFirstPlayerController();
	if (!Joueur || !Joueur->PlayerCameraManager)
	{
		return;
	}
	const FVector Camera = Joueur->PlayerCameraManager->GetCameraLocation();
	const APawn* Moi = Joueur->GetPawn();

	for (int32 I = 0; I < Passants.Num(); I++)
	{
		APalmeraPassant* P = Passants[I];
		FMarche& M = Marches[I];
		const FVector Pos = P->GetActorLocation();
		const double Distance = FVector::Dist2D(Pos, Camera) / Metre;
		if (Distance > 240.0)
		{
			Placer(I, Camera, false);
			continue;
		}

		// Une voiture arrive droit sur lui : il s'écarte en courant, une seule fois
		if (M.Peur <= 0.0 && Moi && Moi->IsA<APalmeraVoiture>())
		{
			const FVector V = Moi->GetVelocity();
			const double VitesseAuto = V.Size2D();
			if (VitesseAuto > 700.0)
			{
				const FVector Sens = V.GetSafeNormal2D();
				const FVector Cote(-Sens.Y, Sens.X, 0.0);
				const FVector Ecart = Pos - Moi->GetActorLocation();
				const double Devant = FVector::DotProduct(Ecart, Sens);
				const double Lateral = FVector::DotProduct(Ecart, Cote);
				if (Devant > 0.0 && Devant < VitesseAuto * 1.3 && FMath::Abs(Lateral) < 190.0)
				{
					M.Peur = 1.2;
					M.Fuite = Cote * (Lateral >= 0.0 ? 1.0 : -1.0);
				}
			}
		}
		if (M.Peur > 0.0)
		{
			M.Peur -= Dt;
			P->Marcher(M.Fuite, 430.0f);
			continue;
		}

		M.Position += M.Sens * M.Vitesse * Dt;
		const FVector2D But = PointTour(M, M.Position + M.Sens * 2.0);
		const FVector Direction = (EnMetres(But.X, But.Y, 0.0) - FVector(Pos.X, Pos.Y, 0.0)).GetSafeNormal();
		P->Marcher(Direction, float(M.Vitesse * Metre));
	}
}
