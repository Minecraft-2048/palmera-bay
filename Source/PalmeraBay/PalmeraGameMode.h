#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PalmeraGameMode.generated.h"

UCLASS()
class PALMERABAY_API APalmeraGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APalmeraGameMode();

	virtual void BeginPlay() override;

	// Écrit le squelette de référence d'un modèle dans un fichier : sert à ajuster les humains de MakeHuman.
	UFUNCTION(BlueprintCallable, Category = "Palmera")
	static void ReleverSquelette(const FString& Modele, const FString& Fichier);
};
