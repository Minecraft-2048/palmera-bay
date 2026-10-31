#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PalmeraPNJ.generated.h"

// L'apparence et l'identité d'un personnage de l'histoire.
struct FFichePNJ
{
	FString Id;
	FString Nom;
	bool bFemme = false;
	FString Modele = TEXT("Sam");	// humain de /Game/Humains
	FLinearColor Haut = FLinearColor(0.2f, 0.3f, 0.5f);
	bool bRecolorer = false;	// le haut de la tenue prend la couleur ci-dessus
	FString Voix = TEXT("paul");
	float Hauteur = 1.0f;		// hauteur de la voix
};

// Un personnage nommé : il attend à sa place, se tourne vers le joueur et parle.
UCLASS()
class PALMERABAY_API APalmeraPNJ : public ACharacter
{
	GENERATED_BODY()

public:
	APalmeraPNJ();

	void Configurer(const FFichePNJ& NouvelleFiche);
	const FFichePNJ& LireFiche() const { return Fiche; }
	void Regarder(const FVector& Point);
	void Marcher(const FVector& Direction, float Vitesse);
	FVector PositionTete() const;

private:
	FFichePNJ Fiche;
};

namespace Palmera
{
	// Donne à un personnage son corps, son visage et sa tenue : le modèle partage le squelette et les animations du jeu.
	void Habiller(USkeletalMeshComponent* Corps, const FFichePNJ& Fiche);
}
