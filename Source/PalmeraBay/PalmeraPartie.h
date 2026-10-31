#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraPartie.generated.h"

class APalmeraJoueur;
class APalmeraPolice;
class UStaticMeshComponent;

// Un lieu de la ville, avec son nom et sa position en mètres.
struct FLieu
{
	FString Nom;
	FVector2D Position;
	FLinearColor Couleur;
};

// L'état de la partie : argent, recherche, mission en cours, cassettes, messages à l'écran.
UCLASS()
class PALMERABAY_API APalmeraPartie : public AActor
{
	GENERATED_BODY()

public:
	APalmeraPartie();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static APalmeraPartie* Lire(const UObject* Contexte);
	static const TArray<FLieu>& Lieux();

	void Annoncer(const FString& Texte, double Duree = 4.0);
	void PlacerBalise(const FVector2D& Position, double Rayon, const FLinearColor& Couleur);
	void CacherBalise();
	void Gagner(int32 Montant);
	void Sauvegarder();

	// Nom du quartier sous cette position.
	static FString NomQuartier(const FVector2D& Position);

	int32 Argent = 500;
	int32 Recherche = 0;
	double Chaleur = 0.0;			// 0 à 100 : ce qui fait monter la recherche

	FString Objectif;				// consigne affichée en haut de l'écran
	FString Message;				// grande annonce temporaire
	FString Aide;					// proposition à l'écran : réparer, repeindre
	double FinMessage = 0.0;
	double Chrono = -1.0;			// compte à rebours de la mission, négatif si aucun

	bool bCibleActive = false;
	FVector2D Cible = FVector2D::ZeroVector;
	bool bEnMission = false;
	int32 MissionsFinies = 0;

	TArray<FVector2D> Cassettes;
	TArray<int32> IdCassettes;
	int32 CassettesTrouvees = 0;
	int32 ChapitreSauve = 0;
	TArray<FString> DrapeauxSauves;
	TArray<FVector2D> DepartsMissions;
	TArray<FString> NomsMissions;

	// Une faute du joueur : fait monter la recherche.
	void Faute(double Gravite);

	void Arrete();
	void Hopital();

private:
	void CreerBalises();
	void ActualiserMission(double Dt);
	void DemarrerMission(int32 Index);
	void EtapeSuivante();
	void Reussir(int32 Recompense);
	void Echouer(const FString& Raison);
	void ActualiserCassettes();

	struct FEtape
	{
		FVector2D Point;
		double Rayon;
		double Temps;
		FString Consigne;
		int32 Sorte;	// 0 : aller, 1 : à pied ou en voiture, 2 : déclenche la recherche
	};

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Balise;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BalisesMissions;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> MaillesCassettes;

	TArray<FEtape> Etapes;
	int32 EtapeActuelle = -1;
	int32 MissionActuelle = -1;
	int32 Recompense = 0;
	double VieVoitureDepart = 0.0;
	double TempsArrestation = 0.0;
	bool bCassettesTrouvees[32] = { false };
};
