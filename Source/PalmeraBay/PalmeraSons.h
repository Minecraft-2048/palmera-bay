#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PalmeraSons.generated.h"

class UAudioComponent;
class USoundBase;

// Tout ce qu'on entend : moteur et pneus de la voiture du joueur, ambiances, sirènes, radios.
UCLASS()
class PALMERABAY_API APalmeraSons : public AActor
{
	GENERATED_BODY()

public:
	APalmeraSons();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static APalmeraSons* Lire(const UObject* Contexte);

	void StationSuivante();
	void Klaxon();
	void Jouer(const TCHAR* Nom, float Volume = 1.0f);
	// Joue la voix de la réplique de ce numéro, si elle a été enregistrée.
	void DireVoix(int32 Numero, float Hauteur);
	FString NomStation() const;

private:
	UAudioComponent* Boucle(const TCHAR* Nom);

	UPROPERTY() TObjectPtr<UAudioComponent> MoteurBas;
	UPROPERTY() TObjectPtr<UAudioComponent> MoteurMoyen;
	UPROPERTY() TObjectPtr<UAudioComponent> MoteurHaut;
	UPROPERTY() TObjectPtr<UAudioComponent> Vent;
	UPROPERTY() TObjectPtr<UAudioComponent> Crissement;
	UPROPERTY() TObjectPtr<UAudioComponent> Sirene;
	UPROPERTY() TObjectPtr<UAudioComponent> Vagues;
	UPROPERTY() TObjectPtr<UAudioComponent> Ville;
	UPROPERTY() TObjectPtr<UAudioComponent> Pluie;
	UPROPERTY() TObjectPtr<UAudioComponent> Radio;
	UPROPERTY() TObjectPtr<UAudioComponent> VoixEnCours;

	int32 Station = 0;		// 0 : radio éteinte
	double VolumeRadio = 0.0;
	double DernierChoc = 0.0;
};
