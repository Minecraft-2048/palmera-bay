#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraPilote.h"
#include "PalmeraPNJ.h"
#include "PalmeraHistoire.generated.h"

class ACameraActor;
class APalmeraJoueur;
class APalmeraPartie;
class APalmeraVoiture;

struct FReplique
{
	FString Qui;
	FString Texte;
	int32 Numero = 0;	// rang dans le fichier, sert à retrouver la voix enregistrée
};

struct FChoixHistoire
{
	FString Texte;
	FString Drapeau;
	int32 Argent = 0;
	TArray<FReplique> Suite;
};

enum class ESorteEtape : uint8
{
	Scene, Appel, Aller, Parler, Suivre, Detruire, Proteger, Semer, Course, Infiltrer, Placer, Attendre, Recherche, Fin
};

struct FEtapeHistoire
{
	ESorteEtape Sorte = ESorteEtape::Attendre;
	FString Consigne;
	FString Pnj;
	FString Si;			// étape jouée seulement si ce drapeau existe
	FString SiNon;		// étape sautée si ce drapeau existe
	FString Raison;		// message d'échec
	FVector2D Point = FVector2D::ZeroVector;
	TArray<FVector2D> Route;
	TArray<FReplique> Lignes;
	TArray<FChoixHistoire> Choix;
	double Rayon = 10.0;
	double Temps = -1.0;
	double Vitesse = 14.0;
	double Vie = 100.0;
	double Heure = -1.0;
	double Pluie = -1.0;
	int32 Nombre = 0;
	int32 Argent = 0;
	int32 Niveau = 0;
	bool bChef = false;
	bool bReprise = false;	// point de reprise après un échec
	bool bCacher = false;
	bool bAPied = false;
};

struct FChapitre
{
	FString Titre;
	TArray<FEtapeHistoire> Etapes;
};

// Une voiture qui suit un trajet fixe sans physique : cible d'une filature, rival de course, alliée à protéger.
struct FTrajet
{
	TArray<FVector2D> Points;
	TArray<double> Cumul;
	double Longueur = 0.0;
	double S = 0.0;
	double Vitesse = 14.0;

	void Construire(const TArray<FVector2D>& Route, double DecalageDroite);
	FVector2D Position(double Distance, double& Cap) const;
	bool Fini() const { return S >= Longueur; }
};

// Déroule le scénario écrit dans Content/Histoire/histoire.json : scènes, appels, poursuites, infiltrations, choix.
UCLASS()
class PALMERABAY_API APalmeraHistoire : public AActor
{
	GENERATED_BODY()

public:
	APalmeraHistoire();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static APalmeraHistoire* Lire(const UObject* Contexte);

	// Dialogue en cours, pour l'interface et les commandes du joueur.
	bool DialogueActif() const { return Lignes.IsValidIndex(IndexLigne) || bChoixOuvert; }
	bool SceneActive() const { return bScene; }
	bool ChoixOuvert() const { return bChoixOuvert; }
	FString NomParleur() const;
	FLinearColor CouleurParleur() const;
	FString TexteAffiche() const;
	const TArray<FString>& LignesDeChoix() const { return TextesChoix; }
	int32 ChoixSelectionne() const { return Selection; }
	void Avancer();
	void DeplacerSelection(int32 Sens);

	// Ce que l'interface affiche autour du jeu.
	FString TitreEcran;
	FString SousTitreEcran;
	double FinTitre = 0.0;
	FString Invite;			// « E : parler à Nina »
	double Detection = -1.0;	// 0 à 1 pendant une infiltration
	double VieAlliee = -1.0;	// 0 à 1 pendant une protection
	double VieCible = -1.0;		// 0 à 1 contre un chef
	FString NomCible;

	// Vrai pendant une étape qui ne laisse pas le joueur faire autre chose.
	bool EstOccupee() const;
	int32 ChapitreActuel() const { return Chapitre; }
	int32 NombreChapitres() const { return Chapitres.Num(); }
	FString NomChapitre() const;
	FString Resume() const;
	const TArray<FFichePNJ>& LireFiches() const { return Fiches; }
	bool EstTerminee() const { return Chapitre >= Chapitres.Num(); }

	void JouerCassette(int32 Rang);

	// Essai sans joueur : déroule toutes les étapes en sautant les trajets et note ce qui se passe.
	void TestAutomatique(double Dt);
	bool bTestAuto = false;

	// Essai au vrai clavier (-testpas) : F9 fait comme si l'objectif de l'étape était atteint, le reste se joue normalement.
	void SauterEtape(bool bAgir);
	bool bTestPas = false;

	// Saute directement à un chapitre (essais).
	void AllerAuChapitre(int32 Index);

	TArray<FString> Drapeaux;

private:
	struct FEnnemi
	{
		TObjectPtr<APalmeraVoiture> Voiture;
		FPiloteAuto Pilote;
	};
	struct FGarde
	{
		TObjectPtr<APalmeraPNJ> Corps;
		FVector2D A, B;
		bool bAller = true;
		double Pause = 0.0;
	};

	bool Charger();
	FFichePNJ FicheDe(const FString& Id) const;
	APalmeraPNJ* PNJ(const FString& Id);
	APalmeraPartie* Partie() const;
	APalmeraJoueur* Joueur() const;

	const FEtapeHistoire* EtapeActuelle() const;
	void DemarrerChapitre();
	void DemarrerEtape();
	void EtapeSuivante();
	void Echouer(const FString& Raison);
	void Nettoyer();

	void LancerLignes(const TArray<FReplique>& Liste, bool bAutomatique);
	void LigneSuivante();
	void FinDesLignes();
	void CadrerScene();
	void Dire(const FReplique& Ligne);

	APalmeraVoiture* CreerVoiture(int32 Modele, const FLinearColor& Peinture, const FVector2D& Position, double Cap, bool bPhysique);
	void SortirEnnemis(int32 Nombre, double Vie, bool bChef);
	void ActualiserEnnemis(double Dt, const FVector& Cible, const AActor* ActeurCible);
	int32 EnnemisEnEtat() const;
	void AvancerTrajet(FTrajet& Trajet, APalmeraVoiture* V, double Dt);
	void ActualiserEtape(double Dt);
	void ActualiserGardes(double Dt);
	FVector2D PlaceLoinDuJoueur(double Min, double Max) const;
	FVector PositionJoueur() const;

	TArray<FChapitre> Chapitres;
	TMap<FString, FFichePNJ> FichesParId;
	TArray<FFichePNJ> Fiches;
	TArray<FString> TextesCassettes;
	int32 PremierNumeroCassette = 0;

	UPROPERTY() TMap<FString, TObjectPtr<APalmeraPNJ>> Personnages;
	UPROPERTY() TObjectPtr<ACameraActor> CameraScene;
	UPROPERTY() TObjectPtr<APalmeraVoiture> VoitureTrajet;
	UPROPERTY() TArray<TObjectPtr<APalmeraVoiture>> Rivaux;

	TArray<FEnnemi> Ennemis;
	TArray<FGarde> Gardes;
	TArray<FTrajet> TrajetsRivaux;
	FTrajet Trajet;

	int32 Chapitre = 0;
	int32 IndexEtape = -1;
	int32 Reprise = 0;
	double TempsEtape = 0.0;
	int32 PointCourse = 0;
	double VieAllieeBrute = 0.0;
	double TempsTropLoin = 0.0;
	double TempsHorsVue = 0.0;

	TArray<FReplique> Lignes;
	int32 IndexLigne = -1;
	double TempsLigne = 0.0;
	bool bAuto = false;
	bool bScene = false;
	bool bChoixOuvert = false;
	bool bChoixFait = false;
	TArray<FString> TextesChoix;
	int32 Selection = 0;
	TArray<TArray<FReplique>> AppelsEnAttente;
	UPROPERTY() TObjectPtr<APalmeraPNJ> InterlocuteurScene;
};
