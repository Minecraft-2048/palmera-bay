#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraVille.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USpotLightComponent;

enum class EQuartier : uint8
{
	Plage,
	Parc,
	Port,
	Centre,
	Residentiel,
	Mixte
};

// Construit toute la ville au lancement de la partie : sol, bâtiments, mobilier et collisions.
// Le plan est toujours le même : chaque bloc tire ses choix d'un générateur réglé sur sa position.
UCLASS()
class PALMERABAY_API APalmeraVille : public AActor
{
	GENERATED_BODY()

public:
	APalmeraVille();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static EQuartier Quartier(int32 I, int32 J);

	// Positions des lampadaires, utilisées pour poser les vraies lumières près du joueur.
	TArray<FVector> Lampadaires;

private:
	UStaticMesh* Maillage(const FString& Dossier, const FString& Nom) const;
	UInstancedStaticMeshComponent* Groupe(const FString& Dossier, const FString& Nom);
	int32 Poser(const FString& Dossier, const FString& Nom, const FVector& Position, double Lacet = 0.0, const TArrayView<const float>& Donnees = TArrayView<const float>());
	void PoserBatiment(const FString& Nom, double X, double Y, double Lacet, FRandomStream& Hasard);
	void PoserPalmier(double X, double Y, FRandomStream& Hasard);
	void PoserVoitureGaree(double X, double Y, double Lacet, FRandomStream& Hasard);
	void PoserMobilier(double X, double Y, double Cote, bool bNordSud, FRandomStream& Hasard);
	void PoserObstacle(double X, double Y, double Rayon, double Hauteur);
	UStaticMeshComponent* Piece(const FString& Dossier, const FString& Nom, const FVector& Position, double Lacet = 0.0, bool bOmbre = true);
	void BoiteCollision(const FVector& Centre, const FVector& DemiTaille, const FRotator& Rotation = FRotator::ZeroRotator);

	void ConstruireSol();
	void ConstruireBloc(int32 I, int32 J);
	void ConstruireRues();

	void ActualiserLumieres();

	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> Groupes;
	UPROPERTY() TArray<TObjectPtr<USpotLightComponent>> Lumieres;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> CollisionsBoites;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> CollisionsPoteaux;

	int32 NbBatiments = 0;
	double ProchaineMaj = 0.0;
	bool bLumieresAllumees = false;
};
