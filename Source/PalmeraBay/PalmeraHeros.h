#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PalmeraHeros.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

// Le personnage à pied : une silhouette habillée par des zones de couleur.
UCLASS()
class PALMERABAY_API APalmeraHeros : public ACharacter
{
	GENERATED_BODY()

public:
	APalmeraHeros();

	virtual void BeginPlay() override;

	void Courir(bool bActif);
	void ActiverCamera(bool bActif);
	void Masquer(bool bCache);

private:
	UPROPERTY() TObjectPtr<USpringArmComponent> Bras;
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
};
