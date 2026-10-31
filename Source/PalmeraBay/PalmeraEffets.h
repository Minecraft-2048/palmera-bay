#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraEffets.generated.h"

class UInstancedStaticMeshComponent;

// Fumée de pneus, poussière de sable et fumée des voitures abîmées : une réserve de disques doux tournés vers la caméra.
UCLASS()
class PALMERABAY_API APalmeraEffets : public AActor
{
	GENERATED_BODY()

public:
	APalmeraEffets();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FBouffee
	{
		FVector Position = FVector::ZeroVector;
		FVector Vitesse = FVector::ZeroVector;
		double Age = 1e9;
		double Vie = 1.0;
		double Taille = 1.0;
		double Force = 0.4;
		double Gris = 0.5;
	};

	void Emettre(const FVector& Position, const FVector& Vitesse, double Vie, double Taille, double Force, double Gris);

	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Disques;

	TArray<FBouffee> Bouffees;
	int32 Prochaine = 0;
	double Accumulateur = 0.0;
};
