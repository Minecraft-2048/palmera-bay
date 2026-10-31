#include "PalmeraPNJ.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void Palmera::Habiller(USkeletalMeshComponent* Corps, const FFichePNJ& Fiche)
{
	const FString Nom = TEXT("SKM_") + Fiche.Modele;
	if (USkeletalMesh* Maillage = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Humains/%s.%s"), *Nom, *Nom)))
	{
		Corps->SetSkeletalMesh(Maillage);
	}
	if (UClass* Anim = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")))
	{
		Corps->SetAnimInstanceClass(Anim);
	}
	const int32 Tenue = Corps->GetMaterialIndex(TEXT("vetement"));
	if (Tenue != INDEX_NONE && Fiche.bRecolorer)
	{
		if (UMaterialInstanceDynamic* Dyn = Corps->CreateDynamicMaterialInstance(Tenue))
		{
			Dyn->SetVectorParameterValue(TEXT("Haut"), Fiche.Haut);
		}
	}
}

APalmeraPNJ::APalmeraPNJ()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 90.0f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->bOrientRotationToMovement = true;
	M->RotationRate = FRotator(0, 360, 0);
	M->MaxWalkSpeed = 150.0f;
	M->bRunPhysicsWithNoController = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void APalmeraPNJ::Configurer(const FFichePNJ& NouvelleFiche)
{
	Fiche = NouvelleFiche;
	Palmera::Habiller(GetMesh(), Fiche);
}

void APalmeraPNJ::Regarder(const FVector& Point)
{
	const FVector Vers = (Point - GetActorLocation()).GetSafeNormal2D();
	if (!Vers.IsNearlyZero())
	{
		SetActorRotation(Vers.Rotation());
	}
}

void APalmeraPNJ::Marcher(const FVector& Direction, float Vitesse)
{
	GetCharacterMovement()->MaxWalkSpeed = Vitesse;
	GetCharacterMovement()->AddInputVector(Direction);
}

FVector APalmeraPNJ::PositionTete() const
{
	return GetActorLocation() + FVector(0, 0, Fiche.bFemme ? 66.0 : 70.0);
}
