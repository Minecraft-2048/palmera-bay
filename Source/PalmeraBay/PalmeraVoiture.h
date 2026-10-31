#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PalmeraVoiture.generated.h"

class UAudioComponent;
class UBoxComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;
class USpotLightComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UStaticMesh;

// Caractéristiques d'un modèle : poids, moteur, tenue de route, et où sont ses pièces.
struct FFicheVoiture
{
	FString Nom;
	FString Caisse;
	FString Vitres;
	FString Roue;
	FVector DemiTaille = FVector(220, 92, 62);
	FVector CentreBoite = FVector(0, 0, 70);
	double Masse = 1400.0;
	double Puissance = 230000.0;	// watts
	double ForceMax = 11000.0;		// newtons
	double Trainee = 0.55;
	double Adherence = 1.2;
	double Raideur = 42000.0;
	double Amortissement = 5200.0;
	double Rayon = 36.0;			// centimètres
	double Empattement = 136.0;		// du centre à chaque essieu
	double Voie = 80.0;				// du centre à chaque roue
	double VoieArriere = -1.0;		// si différente de l'avant
	int32 IndexPeinture = 0;		// emplacement de matériau de la carrosserie
	double HauteurRoue = 36.0;
	double DecalageAvant = 0.0;
	bool bPolice = false;
	bool bTraction = false;			// vrai : roues avant motrices
	bool bQuatreRoues = false;
	FLinearColor Peinture = FLinearColor(0.5f, 0.02f, 0.02f);
};

// Une voiture : jouable, ou posée sur la route quand la circulation la conduit.
UCLASS()
class PALMERABAY_API APalmeraVoiture : public APawn
{
	GENERATED_BODY()

public:
	APalmeraVoiture();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(UPrimitiveComponent* MaPiece, AActor* Autre, UPrimitiveComponent* AutrePiece, bool bMoiBouge, FVector Lieu, FVector Normale, FVector Impulsion, const FHitResult& Touche) override;

	void Configurer(const FFicheVoiture& NouvelleFiche);
	static FFicheVoiture FicheSport();
	static FFicheVoiture FicheToutTerrain();
	// 0 berline, 1 citadine, 2 SUV, 3 fourgon, 4 pick-up, 5 taxi
	static FFicheVoiture FicheCivile(int32 Modele);
	const FFicheVoiture& LireFiche() const { return Fiche; }

	// Consignes du conducteur, de -1 à 1.
	void Commander(double Gaz, double Volant, bool bFrein);
	// Angle des roues que les pneus peuvent tenir à la vitesse actuelle, en degrés.
	double BraquageMax() const;

	// Vrai : les forces des roues s'appliquent. Faux : la voiture est déplacée à la main, sans physique.
	void ActiverPhysique(bool bActif);
	bool PhysiqueActive() const { return bPhysique; }

	// Fait tourner les roues d'une voiture déplacée à la main (vitesse en m/s).
	void AnimerRoues(double Vitesse, double Dt);

	double VitesseKmh() const;
	// Variation de vitesse du dernier choc, en m/s ; s'efface en quelques dixièmes de seconde.
	double ChocRecent() const { return Choc; }
	// Vitesse de glisse latérale la plus forte des quatre roues, en m/s.
	double Derapage() const { return GlisseMax; }
	double GazActuel() const { return FMath::Abs(ConsigneGaz); }
	double Regime() const { return Tours; }
	int32 Rapport() const { return RapportActuel; }
	bool EstRetournee() const;
	void Redresser();

	void AllumerPhares(bool bAllume);
	// Lueur des phares seule, sans projeter de lumière : pour les voitures de la circulation.
	void AllumerLueurs(bool bAllume);
	void AllumerStop(bool bAllume);
	bool PharesAllumes() const { return bPhares; }

	USpringArmComponent* LireBras() const { return Bras; }
	UCameraComponent* LireCamera() const { return Camera; }
	UBoxComponent* LireBoite() const { return Boite; }
	void ActiverCamera(bool bActif);

	// Tourne la caméra autour de la voiture (degrés). Elle revient derrière toute seule si on roule sans y toucher.
	void TournerCamera(double Lacet, double Tangage);
	void RecentrerCamera() { bRecentrer = true; }

	// Santé de la voiture, de 0 à VieMax. À zéro, elle ne roule plus.
	void Abimer(double Points);
	void Reparer();
	double LireVie() const { return Vie; }
	double LireVieMax() const { return VieMax; }
	void DefinirVie(double Valeur) { Vie = VieMax = Valeur; }
	bool EstHorsService() const { return Vie <= 0.0; }

	// Multiplie les dégâts reçus : moins de 1 pour une voiture blindée.
	double FacteurDegats = 1.0;
	bool bEnnemie = false;
	// Vrai pour une voiture d'une scène de l'histoire : le joueur ne peut pas la prendre.
	bool bReservee = false;
	bool bCibleJoueur = false;

private:
	struct FRoue
	{
		FVector Position = FVector::ZeroVector;
		bool bAvant = false;
		bool bDroite = false;
		bool bContact = false;
		double Compression = 0.0;
		double CompressionAvant = 0.0;
		double Rotation = 0.0;
		double Charge = 0.0;
		double Hauteur = 0.0;
		FVector Sol = FVector::UpVector;
		TObjectPtr<UStaticMeshComponent> Maillage;
	};

	void SimulerRoues(double Dt);
	void ActualiserVisuel(double Dt);
	void ActualiserCamera(double Dt);

	UPROPERTY() TObjectPtr<UBoxComponent> Boite;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Carrosserie;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Vitres;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Rampe;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> MaillagesRoues;
	UPROPERTY() TObjectPtr<USpringArmComponent> Bras;
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<USpotLightComponent> PhareGauche;
	UPROPERTY() TObjectPtr<USpotLightComponent> PhareDroit;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatCaisse;

	FFicheVoiture Fiche;
	TArray<FRoue> Roues;

	double ConsigneGaz = 0.0;
	double ConsigneVolant = 0.0;
	bool bConsigneFrein = false;
	double Volant = 0.0;
	double Tours = 900.0;
	int32 RapportActuel = 1;
	bool bPhysique = true;
	bool bPhares = false;
	bool bLueurs = false;
	bool bStop = false;
	double LacetCamera = 0.0;
	double TempsAuSol = 0.0;
	double Choc = 0.0;
	double GlisseMax = 0.0;
	double OrbiteLacet = 0.0;
	double OrbiteTangage = 0.0;
	bool bRecentrer = false;
	double Vie = 100.0;
	double VieMax = 100.0;
};
