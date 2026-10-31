#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PalmeraSauvegarde.generated.h"

UCLASS()
class PALMERABAY_API UPalmeraSauvegarde : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Argent = 500;
	UPROPERTY() int32 MissionsFinies = 0;
	UPROPERTY() int32 CassettesTrouvees = 0;
	UPROPERTY() int32 Chapitre = 0;
	UPROPERTY() TArray<FString> Drapeaux;
};
