#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraLieux.generated.h"

class APalmeraPNJ;
class UInstancedStaticMeshComponent;

// Un bâtiment où l'on peut entrer : sa place dans la ville et le contenu de sa salle, décrits dans Content/Histoire/lieux.json.
struct FSalle
{
	struct FLampe { FVector Position = FVector::ZeroVector; float Force = 0.f; FLinearColor Couleur = FLinearColor::White; float Rayon = 0.f; };
	struct FPersonne { FString Modele; FVector2D Position = FVector2D::ZeroVector; double Cap = 0.0; FString Phrase; };
	struct FAction { FVector2D Position = FVector2D::ZeroVector; double Rayon = 2.0; FString Invite; FString Message; int32 Prix = 0; int32 Gain = 0; };

	FString Id;
	FString Nom;
	FString Enseigne;
	int32 BlocI = 0;
	int32 BlocJ = 0;
	int32 LotA = 0;
	int32 LotB = 0;
	double Lacet = 0.0;
	double Recul = 0.0;			// de la façade au milieu du bâtiment, marches comprises
	FLinearColor Couleur = FLinearColor::White;
	FVector Taille = FVector::ZeroVector;
	TArray<FBox> Boites;
	TArray<FLampe> Lampes;
	TArray<FPersonne> Personnes;
	TArray<FAction> Actions;

	FVector2D CentreLot() const;
	FVector Facade() const;		// direction vers laquelle regarde la façade
	FVector2D Porte() const;
};

// Fait entrer et sortir le joueur des bâtiments : comme une petite scène à part, on y passe par un fondu au noir.
UCLASS()
class PALMERABAY_API APalmeraLieux : public AActor
{
	GENERATED_BODY()

public:
	APalmeraLieux();

	virtual void Tick(float DeltaSeconds) override;

	static const TArray<FSalle>& Salles();
	static const FSalle* SalleDuLot(int32 I, int32 J, int32 A, int32 B);
	static APalmeraLieux* Lire(const UObject* Contexte);

	bool EstDedans() const { return Actuelle != INDEX_NONE; }

	// Ce que l'interface affiche.
	FString Invite;
	FString Bulle;
	double TempsBulle = 0.0;
	double Noir = 0.0;			// 0 à 1 pendant le fondu

private:
	void Entrer(int32 Index);
	void Sortir();
	void Vider();
	void Dire(const FString& Texte);

	static constexpr int32 Aucune = INDEX_NONE;
	static constexpr int32 VersDehors = -2;

	int32 Actuelle = Aucune;
	int32 Prochaine = Aucune;
	double TempsEssai = 2.5;
	FString LieuEssai;
	bool bEssaiPortes = false;
	int32 PorteEssai = 0;
	bool bEssaiDedans = false;

	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Decor;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Pieces;
	UPROPERTY() TArray<TObjectPtr<APalmeraPNJ>> Habitants;
};
