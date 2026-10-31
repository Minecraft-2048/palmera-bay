#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PalmeraJoueur.generated.h"

class APalmeraHeros;
class APalmeraVoiture;
class APalmeraPNJ;
class ACameraActor;
class SPalmeraHud;

// Lit le clavier et la manette, et les transmet au personnage, à la voiture ou aux menus.
UCLASS()
class PALMERABAY_API APalmeraJoueur : public APlayerController
{
	GENERATED_BODY()

public:
	APalmeraJoueur();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Raison) override;
	virtual void PlayerTick(float DeltaTime) override;

	void Demarrer(APalmeraHeros* NouveauHeros);
	APalmeraVoiture* VoitureActuelle() const { return Voiture; }
	APalmeraHeros* LireHeros() const { return Heros; }

	// Replace le joueur à pied à cet endroit (arrestation, hôpital).
	void Ramener(const FVector& Position);
	// Une scène de l'histoire : le joueur descend de voiture et regarde la conversation.
	void CommencerScene(APalmeraPNJ* Avec, ACameraActor* Camera);
	void FinirScene();
	// Vrai pendant l'image où la touche d'action vient d'être pressée.
	bool DemandeAction() const { return bAction; }

	// Pour le test automatique de l'histoire.
	void ForcerAction() { bAction = true; }
	void DescendrePourTest() { if (Voiture) { Descendre(); } }
	void Noter(const FString& Ligne) const;

	// Jeu de course : le joueur commence directement au volant, commandes gelées jusqu'au départ.
	void PrendreVoiture(APalmeraVoiture* NouvelleVoiture);
	void LacherVoiture();
	bool bCommandesGelees = false;

private:
	void CommandesAPied(float Dt);
	void CommandesAuVolant(float Dt);
	void CommandesMenu();
	void Monter();
	void Descendre();
	void BasculerMenu();
	void AppliquerQualite(int32 Niveau);
	bool Touche(FKey A, FKey B = EKeys::Invalid, FKey C = EKeys::Invalid) const;
	bool Appui(FKey A, FKey B = EKeys::Invalid) const;
	void PiloteDeTest(float Dt);
	void ParcoursDeTest(float Dt);
	void EssaiVirage(float Dt);

	UPROPERTY() TObjectPtr<APalmeraHeros> Heros;
	UPROPERTY() TObjectPtr<APalmeraVoiture> Voiture;

	TSharedPtr<SPalmeraHud> Hud;
	TSharedPtr<SWidget> EnveloppeHud;
	bool bAction = false;
	bool bEnScene = false;
	bool bPiloteTest = false;
	double TempsPilote = 0.0;
	double ProchainReleve = 0.0;
	bool bParcours = false;
	bool bEssaiVirage = false;
	bool bReleve = false;
	int32 EtapeParcours = 0;
};
