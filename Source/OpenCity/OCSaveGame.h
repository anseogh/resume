#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "OCSaveGame.generated.h"

UCLASS()
class OPENCITY_API UOCSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 CompletedMissions = 0;
	UPROPERTY() int32 Money = 0;
};
