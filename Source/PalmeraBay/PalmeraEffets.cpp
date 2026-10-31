#include "PalmeraEffets.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "PalmeraConfig.h"
#include "PalmeraVoiture.h"

using namespace Palmera;

namespace
{
	constexpr int32 NombreBouffees = 90;
}

APalmeraEffets::APalmeraEffets()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Racine")));
}

void APalmeraEffets::BeginPlay()
{
	Super::BeginPlay();

	Disques = NewObject<UInstancedStaticMeshComponent>(this);
	Disques->SetupAttachment(RootComponent);
	Disques->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Disques->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiaux/M_Fumee.M_Fumee")));
	Disques->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disques->SetCastShadow(false);
	Disques->NumCustomDataFloats = 2;
	Disques->bAffectDynamicIndirectLighting = false;
	Disques->SetBoundsScale(1000.0f);
	Disques->RegisterComponent();

	Bouffees.SetNum(NombreBouffees);
	const FTransform Cache(FRotator::ZeroRotator, FVector(0, 0, -50000), FVector(0.001));
	for (int32 I = 0; I < NombreBouffees; I++)
	{
		Disques->AddInstance(Cache, true);
	}
}

void APalmeraEffets::Emettre(const FVector& Position, const FVector& Vitesse, double Vie, double Taille, double Force, double Gris)
{
	FBouffee& B = Bouffees[Prochaine];
	Prochaine = (Prochaine + 1) % NombreBouffees;
	B.Position = Position;
	B.Vitesse = Vitesse;
	B.Age = 0.0;
	B.Vie = Vie;
	B.Taille = Taille;
	B.Force = Force;
	B.Gris = Gris;
}

void APalmeraEffets::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	const APlayerController* Joueur = GetWorld()->GetFirstPlayerController();
	if (!Joueur || !Joueur->PlayerCameraManager || !Disques)
	{
		return;
	}
	const FVector Camera = Joueur->PlayerCameraManager->GetCameraLocation();

	Accumulateur += Dt;
	if (Accumulateur > 0.05)
	{
		Accumulateur = 0.0;
		for (TActorIterator<APalmeraVoiture> It(GetWorld()); It; ++It)
		{
			const APalmeraVoiture* V = *It;
			if (V->IsHidden() || !V->PhysiqueActive() || FVector::DistSquared(V->GetActorLocation(), Camera) > FMath::Square(16000.0))
			{
				continue;
			}
			const FVector Pos = V->GetActorLocation();
			const FVector Sol(Pos.X, Pos.Y, FMath::Max(Pos.Z - V->LireFiche().CentreBoite.Z + 25.0, 10.0));
			const bool bSable = Pos.X < BordOuest * Metre - 1500.0;

			// Pneus qui glissent, ou poussière sur le sable dès qu'on roule
			if (V->Derapage() > (bSable ? 2.5 : 6.0))
			{
				const FVector Arriere = Pos - V->GetActorForwardVector() * 150.0;
				const FVector P(Arriere.X + FMath::FRandRange(-60.f, 60.f), Arriere.Y + FMath::FRandRange(-60.f, 60.f), Sol.Z);
				Emettre(P, V->GetVelocity() * 0.12 + FVector(0, 0, 40), bSable ? 1.3 : 0.9, bSable ? 260.0 : 200.0, 0.3, bSable ? 0.34 : 0.62);
			}
			else if (bSable && V->GetVelocity().Size() > 500.0)
			{
				Emettre(FVector(Pos.X, Pos.Y, Sol.Z) - V->GetActorForwardVector() * 180.0, FVector(0, 0, 30), 1.1, 180.0, 0.18, 0.34);
			}

			// Moteur abîmé : fumée qui sort du capot, noire quand la voiture est finie
			const double Etat = V->LireVie() / FMath::Max(V->LireVieMax(), 1.0);
			if (Etat < 0.4)
			{
				const FVector Capot = Pos + V->GetActorForwardVector() * 130.0 + FVector(0, 0, 40);
				Emettre(Capot, V->GetVelocity() * 0.4 + FVector(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), 170), 1.6, 120.0, 0.34, Etat <= 0.0 ? 0.03 : 0.1);
			}
		}
	}

	const FRotator Face = Joueur->PlayerCameraManager->GetCameraRotation();
	for (int32 I = 0; I < NombreBouffees; I++)
	{
		FBouffee& B = Bouffees[I];
		FTransform T(FRotator::ZeroRotator, FVector(0, 0, -50000), FVector(0.001));
		if (B.Age < B.Vie)
		{
			B.Age += Dt;
			B.Position += B.Vitesse * Dt;
			B.Vitesse *= FMath::Max(1.0 - Dt * 1.2, 0.0);
			const double Part = B.Age / B.Vie;
			const double Echelle = B.Taille * (0.4 + Part * 1.6) / 100.0;
			// Le plan du moteur regarde vers +Z : on le couche face à la caméra
			T = FTransform(FRotationMatrix::MakeFromZ((Camera - B.Position).GetSafeNormal()).Rotator(), B.Position, FVector(Echelle));
			const float Donnees[2] = { float(B.Force * (1.0 - Part)), float(B.Gris) };
			Disques->SetCustomData(I, Donnees, false);
		}
		Disques->UpdateInstanceTransform(I, T, true, I == NombreBouffees - 1, true);
	}
}
