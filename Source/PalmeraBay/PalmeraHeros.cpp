#include "PalmeraHeros.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PalmeraPNJ.h"

APalmeraHeros::APalmeraHeros()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -92), FRotator(0, -90, 0));

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* M = GetCharacterMovement();
	M->bOrientRotationToMovement = true;
	M->RotationRate = FRotator(0, 620, 0);
	M->MaxWalkSpeed = 240.0f;
	M->JumpZVelocity = 520.0f;
	M->AirControl = 0.35f;
	M->BrakingDecelerationWalking = 1800.0f;

	Bras = CreateDefaultSubobject<USpringArmComponent>(TEXT("Bras"));
	Bras->SetupAttachment(RootComponent);
	Bras->TargetArmLength = 340.0f;
	Bras->SocketOffset = FVector(0, 48, 62);
	Bras->bUsePawnControlRotation = true;
	Bras->bEnableCameraLag = true;
	Bras->CameraLagSpeed = 14.0f;
	Bras->ProbeSize = 14.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Bras);
	Camera->SetFieldOfView(74.0f);
	Camera->bConstrainAspectRatio = false;
}

void APalmeraHeros::BeginPlay()
{
	Super::BeginPlay();

	Palmera::Habiller(GetMesh(), FFichePNJ());
}

void APalmeraHeros::Courir(bool bActif)
{
	GetCharacterMovement()->MaxWalkSpeed = bActif ? 640.0f : 240.0f;
}

void APalmeraHeros::ActiverCamera(bool bActif)
{
	Camera->SetActive(bActif);
}

void APalmeraHeros::Masquer(bool bCache)
{
	SetActorHiddenInGame(bCache);
	SetActorEnableCollision(!bCache);
	GetCharacterMovement()->SetMovementMode(bCache ? MOVE_None : MOVE_Walking);
}
