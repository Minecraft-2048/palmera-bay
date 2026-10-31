#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraCirculation.generated.h"

class APalmeraVoiture;

// Les voitures qui roulent seules : elles suivent leur voie, respectent les feux et gardent leurs distances.
// Elles ne vivent qu'autour du joueur ; celles qui s'éloignent reviennent ailleurs, hors de sa vue.
UCLASS()
class PALMERABAY_API APalmeraCirculation : public AActor
{
	GENERATED_BODY()

public:
	APalmeraCirculation();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Temps du cycle des feux, de 0 à 24 secondes.
	double TempsFeux = 0.0;

	// Vrai si les voitures de cet axe (0 : nord-sud, 1 : est-ouest) ont le vert à l'instant T.
	static bool FeuVert(int32 Axe, double T);

	UPROPERTY() TArray<TObjectPtr<APalmeraVoiture>> Voitures;

private:
	struct FConducteur
	{
		int32 Axe = 0;		// 0 : rue nord-sud (X constant), 1 : rue est-ouest (Y constant)
		int32 Rue = 0;
		int32 Sens = 1;		// +1 : vers +Y ou +X
		int32 Voie = 0;		// 0 : près du centre, 1 : près du trottoir
		double S = 0.0;		// position le long de la rue, en mètres
		double Vitesse = 0.0;
		double VitesseVoulue = 12.0;
		bool bVirage = false;
		FVector2D P0, P1, P2;
		double T = 0.0;
		double LongueurVirage = 1.0;
		int32 AxeApres = 0, RueApres = 0, SensApres = 1, VoieApres = 0;
		double SApres = 0.0;
		bool bAccidente = false;
		double TempsAccident = 0.0;
	};

	void Placer(int32 Index, bool bPremierTour);
	bool TrouverPlace(FConducteur& C, const FVector& Autour, double RayonMin, double RayonMax, const FVector& Regard);
	void PreparerCarrefour(FConducteur& C, int32 Carrefour);
	FVector2D PointVoie(int32 Axe, int32 Rue, int32 Sens, int32 Voie, double S) const;
	void Avancer(FConducteur& C, APalmeraVoiture* V, double Pas, double Dt);
	double DistanceDevant(int32 Index, const FVector& Position, const FVector& Direction) const;

	UFUNCTION()
	void SurChoc(AActor* Soi, AActor* Autre, FVector Impulsion, const FHitResult& Touche);

	TArray<FConducteur> Conducteurs;
	UPROPERTY() TArray<TObjectPtr<APalmeraVoiture>> Autres;
	FRandomStream Hasard;
	int32 NbVoitures = 46;
};
