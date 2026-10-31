#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraMonde.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UMaterialParameterCollection;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

// Le ciel, le soleil et la lune, les nuages, la brume, le réglage de l'image et la météo.
UCLASS()
class PALMERABAY_API APalmeraMonde : public AActor
{
	GENERATED_BODY()

public:
	APalmeraMonde();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Heure de la journée, de 0 à 24.
	double Heure = 17.6;
	// Heures de jeu par seconde réelle (1 heure par minute).
	double VitesseTemps = 1.0 / 60.0;
	double Pluie = 0.0;
	double PluieCible = 0.0;
	bool bMeteoAuto = true;

	// 0 en plein jour, 1 en pleine nuit.
	double Nuit = 0.0;

private:
	void Actualiser(double Dt);

	UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY() TObjectPtr<UVolumetricCloudComponent> Nuages;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Soleil;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Lune;
	UPROPERTY() TObjectPtr<USkyLightComponent> LumiereCiel;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Brume;
	UPROPERTY() TObjectPtr<UPostProcessComponent> Image;
	UPROPERTY() TObjectPtr<UMaterialParameterCollection> Parametres;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Gouttes;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Voute;

	double TempsMeteo = 180.0;
	bool bOmbresSoleil = true;
};
