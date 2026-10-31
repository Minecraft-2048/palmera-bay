#include "PalmeraMonde.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	double Lisser(double A, double B, double X)
	{
		const double T = FMath::Clamp((X - A) / (B - A), 0.0, 1.0);
		return T * T * (3.0 - 2.0 * T);
	}
}

APalmeraMonde::APalmeraMonde()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	USceneComponent* Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	Racine->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Racine);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Racine);
	Atmosphere->SetMobility(EComponentMobility::Movable);

	Nuages = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Nuages"));
	Nuages->SetupAttachment(Racine);

	Soleil = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Soleil"));
	Soleil->SetupAttachment(Racine);
	Soleil->SetMobility(EComponentMobility::Movable);
	Soleil->SetIntensity(120000.0f);
	Soleil->SetAtmosphereSunLight(true);
	Soleil->SetAtmosphereSunLightIndex(0);
	Soleil->SetForwardShadingPriority(1);
	Soleil->SetLightSourceAngle(0.53f);
	Soleil->bCastCloudShadows = true;
	Soleil->SetCastVolumetricShadow(true);

	Lune = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Lune"));
	Lune->SetupAttachment(Racine);
	Lune->SetMobility(EComponentMobility::Movable);
	Lune->SetIntensity(5.0f);
	Lune->SetLightColor(FLinearColor(0.55f, 0.66f, 1.0f));
	Lune->SetAtmosphereSunLight(true);
	Lune->SetAtmosphereSunLightIndex(1);
	Lune->SetCastShadows(false);

	LumiereCiel = CreateDefaultSubobject<USkyLightComponent>(TEXT("LumiereCiel"));
	LumiereCiel->SetupAttachment(Racine);
	LumiereCiel->SetMobility(EComponentMobility::Movable);
	LumiereCiel->SetRealTimeCaptureEnabled(true);
	LumiereCiel->bLowerHemisphereIsBlack = false;
	LumiereCiel->SetLowerHemisphereColor(FLinearColor(0.02f, 0.02f, 0.025f));

	Brume = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Brume"));
	Brume->SetupAttachment(Racine);
	Brume->SetFogDensity(0.012f);
	Brume->SetFogHeightFalloff(0.16f);
	Brume->SetVolumetricFog(true);
	Brume->SetVolumetricFogScatteringDistribution(0.75f);
	Brume->SetVolumetricFogExtinctionScale(0.6f);

	Image = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Image"));
	Image->SetupAttachment(Racine);
	Image->bUnbound = true;
	FPostProcessSettings& P = Image->Settings;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = AEM_Histogram;
	P.bOverride_AutoExposureMinBrightness = true;
	P.AutoExposureMinBrightness = 3.2f;
	P.bOverride_AutoExposureMaxBrightness = true;
	P.AutoExposureMaxBrightness = 15.0f;
	P.bOverride_AutoExposureSpeedUp = true;
	P.AutoExposureSpeedUp = 1.5f;
	P.bOverride_AutoExposureSpeedDown = true;
	P.AutoExposureSpeedDown = 1.5f;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = 0.1f;
	P.bOverride_BloomIntensity = true;
	P.BloomIntensity = 0.55f;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = 0.28f;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = 0.02f;
	P.bOverride_MotionBlurAmount = true;
	P.MotionBlurAmount = 0.35f;
	P.bOverride_MotionBlurMax = true;
	P.MotionBlurMax = 2.5f;
	P.bOverride_SceneFringeIntensity = true;
	P.SceneFringeIntensity = 0.15f;
	P.bOverride_ColorSaturation = true;
	P.ColorSaturation = FVector4(1.0, 1.0, 1.0, 1.02);
	P.bOverride_ColorContrast = true;
	P.ColorContrast = FVector4(1.0, 1.0, 1.0, 1.06);
	P.bOverride_LumenFinalGatherQuality = true;
	P.LumenFinalGatherQuality = 2.0f;
	P.bOverride_LumenReflectionQuality = true;
	P.LumenReflectionQuality = 1.5f;
	P.bOverride_LumenFrontLayerTranslucencyReflections = true;
	P.LumenFrontLayerTranslucencyReflections = true;
}

void APalmeraMonde::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* Nuage = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")))
	{
		Nuages->SetMaterial(Nuage);
	}
	Parametres = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiaux/MPC_Palmera.MPC_Palmera"));

	// Voûte céleste : le soleil, la lune et les étoiles se dessinent sur cette sphère
	Voute = NewObject<UStaticMeshComponent>(this);
	Voute->SetupAttachment(RootComponent);
	Voute->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Voute->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiaux/M_Ciel.M_Ciel")));
	Voute->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Voute->SetCastShadow(false);
	Voute->bAffectDynamicIndirectLighting = false;
	Voute->SetWorldScale3D(FVector(300000.0));
	Voute->RegisterComponent();

	Gouttes = NewObject<UStaticMeshComponent>(this);
	Gouttes->SetupAttachment(RootComponent);
	Gouttes->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Modeles/Mobilier/SM_Pluie/StaticMeshes/SM_Pluie.SM_Pluie")));
	Gouttes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Gouttes->SetCastShadow(false);
	Gouttes->SetBoundsScale(40.0f);
	Gouttes->bAffectDynamicIndirectLighting = false;
	Gouttes->SetVisibility(false);
	Gouttes->RegisterComponent();

	FParse::Value(FCommandLine::Get(), TEXT("-heure="), Heure);
	double PluieDemandee = -1.0;
	if (FParse::Value(FCommandLine::Get(), TEXT("-pluie="), PluieDemandee))
	{
		Pluie = PluieCible = PluieDemandee;
		bMeteoAuto = false;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("tempsfige")))
	{
		VitesseTemps = 0.0;
	}
	Actualiser(0.0);
}

void APalmeraMonde::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Actualiser(DeltaSeconds);
}

void APalmeraMonde::Actualiser(double Dt)
{
	Heure = FMath::Fmod(Heure + Dt * VitesseTemps, 24.0);

	// Le soleil se lève à l'est (+X) à 6 h et se couche à l'ouest (-X) à 18 h, en passant par le sud (+Y).
	const double Angle = (Heure - 6.0) / 12.0 * UE_DOUBLE_PI;
	const FVector VersSoleil = FVector(FMath::Cos(Angle), 0.34, FMath::Sin(Angle)).GetSafeNormal();
	const FVector VersLune = FVector(-FMath::Cos(Angle) * 0.9, -0.25, -FMath::Sin(Angle)).GetSafeNormal();
	const double Elevation = VersSoleil.Z;

	Soleil->SetWorldRotation(FRotationMatrix::MakeFromX(-VersSoleil).Rotator());
	Lune->SetWorldRotation(FRotationMatrix::MakeFromX(-VersLune).Rotator());

	Nuit = 1.0 - Lisser(-0.12, 0.04, Elevation);

	const bool bOmbres = Elevation > -0.07;
	if (bOmbres != bOmbresSoleil)
	{
		bOmbresSoleil = bOmbres;
		Soleil->SetCastShadows(bOmbres);
		Lune->SetCastShadows(!bOmbres);
		Soleil->SetForwardShadingPriority(bOmbres ? 1 : 0);
		Lune->SetForwardShadingPriority(bOmbres ? 0 : 1);
	}

	// La météo change d'elle-même : quelques averses entre de longues éclaircies.
	if (bMeteoAuto)
	{
		TempsMeteo -= Dt;
		if (TempsMeteo <= 0.0)
		{
			if (PluieCible > 0.0)
			{
				PluieCible = 0.0;
				TempsMeteo = FMath::FRandRange(240.0, 420.0);
			}
			else
			{
				PluieCible = FMath::FRand() < 0.4f ? FMath::FRandRange(0.6, 1.0) : 0.0;
				TempsMeteo = FMath::FRandRange(90.0, 180.0);
			}
		}
	}
	Pluie = FMath::FInterpConstantTo(Pluie, PluieCible, Dt, 0.05);

	Soleil->SetIntensity(120000.0f * float(1.0 - Pluie * 0.75));
	if (Gouttes)
	{
		const bool bPluie = Pluie > 0.03;
		if (bPluie != Gouttes->IsVisible())
		{
			Gouttes->SetVisibility(bPluie);
		}
		if (bPluie)
		{
			if (const APlayerController* Joueur = GetWorld()->GetFirstPlayerController())
			{
				if (Joueur->PlayerCameraManager)
				{
					Gouttes->SetWorldLocation(Joueur->PlayerCameraManager->GetCameraLocation());
				}
			}
		}
	}
	Brume->SetFogDensity(float(FMath::Lerp(0.012, 0.04, Pluie)));

	if (Parametres)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, Parametres, TEXT("Nuit"), float(Nuit));
		UKismetMaterialLibrary::SetScalarParameterValue(this, Parametres, TEXT("Pluie"), float(Pluie));
	}
}
