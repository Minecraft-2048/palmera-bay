#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "PalmeraPietons.generated.h"

class UStaticMeshComponent;

// Un passant : marche sur le trottoir autour d'un bloc, s'écarte d'un bond quand une voiture fonce sur lui.
UCLASS()
class PALMERABAY_API APalmeraPassant : public ACharacter
{
	GENERATED_BODY()

public:
	APalmeraPassant();

	void Habiller(FRandomStream& Hasard);
	void Marcher(const FVector& Direction, float Vitesse);

private:
};

// Fait vivre les trottoirs autour du joueur. Les passants trop loin reviennent ailleurs, hors de sa vue.
UCLASS()
class PALMERABAY_API APalmeraPietons : public AActor
{
	GENERATED_BODY()

public:
	APalmeraPietons();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TArray<TObjectPtr<APalmeraPassant>> Passants;

private:
	struct FMarche
	{
		FVector2D Centre = FVector2D::ZeroVector;	// centre du bloc
		double Position = 0.0;						// distance parcourue autour du bloc
		int32 Sens = 1;
		double Vitesse = 1.3;
		double Peur = 0.0;
		FVector Fuite = FVector::ZeroVector;		// direction d'écart quand une voiture arrive
	};

	void Placer(int32 Index, const FVector& Autour, bool bPremierTour);
	FVector2D PointTour(const FMarche& M, double S) const;

	TArray<FMarche> Marches;
	FRandomStream Hasard;
	int32 NbPassants = 26;
};
