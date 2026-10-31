#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraCourse.generated.h"

class APalmeraVoiture;

enum class EEtatCourse : uint8
{
	Menu, Depart, Course, Resultats
};

// Un circuit fermé dans les rues : la suite des carrefours par lesquels il passe.
struct FCircuit
{
	FString Nom;
	double Heure = 17.6;
	double Pluie = 0.0;
	int32 Tours = 3;
	TArray<FIntPoint> Noeuds;
};

struct FConcurrent
{
	TObjectPtr<APalmeraVoiture> Voiture;
	FString Nom;
	FLinearColor Couleur = FLinearColor::White;
	double Talent = 0.95;
	double Decalage = 0.0;		// écart au milieu de la route
	int32 Index = 0;			// point du tracé le plus proche
	int32 Tours = 0;
	bool bMoitie = false;		// a passé la moitié du tour en cours
	double Arrivee = -1.0;
	double Bloque = 0.0;
	double Progression() const;
	double S = 0.0;
};

// Palmera Racing : le jeu de course, lancé avec -course. Mêmes rues et mêmes voitures, circuits fermés par des barrières.
UCLASS()
class PALMERABAY_API APalmeraCourse : public AActor
{
	GENERATED_BODY()

public:
	APalmeraCourse();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static APalmeraCourse* Lire(const UObject* Contexte);

	// Ce que l'interface affiche.
	EEtatCourse Etat = EEtatCourse::Menu;
	int32 ChoixLigne = 0;
	int32 ChoixCircuit = 0;
	int32 ChoixVoiture = 0;
	int32 ToursChoisis = 3;
	double Compte = 0.0;
	double Chrono = 0.0;
	double TempsTour = 0.0;
	double MeilleurTour = -1.0;
	bool bNouveauRecord = false;

	const TArray<FCircuit>& Circuits() const { return ListeCircuits; }
	FString NomVoiture() const;
	double Record() const;
	int32 Rang() const;
	const FConcurrent* Joueur() const { return Concurrents.Num() > 0 ? &Concurrents[0] : nullptr; }
	TArray<const FConcurrent*> Classement() const;

	// Le tracé, pour la mini-carte.
	const TArray<FVector2D>& LireTrace() const { return Points; }

private:
	void Preparer();
	void Vider();
	void ConstruireTrace(const FCircuit& C);
	void PoserBarrieres(const FCircuit& C);
	void PoserPiece(const TCHAR* Nom, const FVector& Position, double Lacet, const FVector& DemiBoite);
	APalmeraVoiture* CreerVoiture(int32 Modele, const FLinearColor& Peinture, const FVector2D& Position, double Cap);
	void Avancer(FConcurrent& C);
	void Piloter(FConcurrent& C, double Dt);
	void CommandesMenu();
	void Finir();
	FVector2D Point(int32 Index, double Decalage) const;
	double Cap(int32 Index) const;

	TArray<FCircuit> ListeCircuits;
	TArray<FVector2D> Points;
	TArray<double> Cumul;
	TArray<double> VMax;
	double Longueur = 0.0;
	TArray<FConcurrent> Concurrents;
	double ApresArrivee = 0.0;
	bool bEssai = false;

	UPROPERTY() TArray<TObjectPtr<UActorComponent>> Pieces;
	UPROPERTY() TArray<TObjectPtr<APalmeraVoiture>> Voitures;
};
