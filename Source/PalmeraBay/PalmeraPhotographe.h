#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraPhotographe.generated.h"

class ACameraActor;

// Outil de vérification : place une caméra, attend que l'image soit prête, enregistre une photo et quitte.
//   -photo=C:\dossier\image.png -cam=x,y,z,lacet,tangage (en mètres et degrés) -attente=secondes
//   -serie=C:\dossier\liste.txt : une photo par ligne « nom;x,y,z,lacet,tangage[,champ];heure[;pluie] »
UCLASS()
class PALMERABAY_API APalmeraPhotographe : public AActor
{
	GENERATED_BODY()

public:
	APalmeraPhotographe();

	// Vrai si la ligne de commande demande une photo.
	static bool EstDemande();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FPrise
	{
		FString Chemin;
		FString Vue;
		double Heure = -1.0;
		double Pluie = -1.0;
	};

	void Preparer(int32 Index);
	void PlacerCamera(const FString& Vue);

	UPROPERTY() TObjectPtr<ACameraActor> Camera;

	TArray<FPrise> Prises;
	int32 Courante = 0;
	double Attente = 12.0;
	double Calme = 0.0;
	double Total = 0.0;
	double TempsImages = 0.0;
	int32 NbImages = 0;
	int32 Etape = 0;
};
