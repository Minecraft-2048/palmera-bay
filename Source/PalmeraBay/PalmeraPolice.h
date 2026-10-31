#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraPilote.h"
#include "PalmeraPolice.generated.h"

class APalmeraVoiture;
class UPointLightComponent;

// Les voitures de police : elles sortent de la réserve quand la recherche monte et poursuivent le joueur
// en suivant les rues, puis en ligne droite dès qu'elles le voient.
UCLASS()
class PALMERABAY_API APalmeraPolice : public AActor
{
	GENERATED_BODY()

public:
	APalmeraPolice();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TArray<TObjectPtr<APalmeraVoiture>> Patrouilles;

	// Vrai si une voiture de police voit le joueur.
	bool bJoueurVu = false;
	// Distance de la voiture la plus proche, en mètres.
	double DistancePlusProche = 1e9;

private:
	struct FEtat
	{
		bool bActive = false;
		FPiloteAuto Pilote;
	};

	void Sortir(int32 Index);
	void Ranger(int32 Index);
	void Piloter(int32 Index, const FVector& Cible, double Dt);

	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Gyrophares;

	TArray<FEtat> Etats;
	double TempsAppel = 0.0;
	double TempsJournal = 0.0;
	FRandomStream Hasard;
};
