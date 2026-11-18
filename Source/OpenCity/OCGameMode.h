#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OCGameMode.generated.h"

class AOCCityGenerator;
class AOCWorldEnvironment;

/**
 * Bootstraps the whole game into any (even empty) level: generates the city, spawns the
 * sky/sun, spawns the player on a sidewalk and handles death/respawn.
 */
UCLASS()
class OPENCITY_API AOCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOCGameMode();

	static AOCGameMode* Get(const UObject* WorldContext);

	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	AOCCityGenerator* GetCity() const { return City; }
	AOCWorldEnvironment* GetEnvironment() const { return Environment; }

	void HandlePlayerDeath(APlayerController* PC);

private:
	void EnsureWorld();
	void RespawnPlayer(TWeakObjectPtr<APlayerController> PC);

	UPROPERTY() TObjectPtr<AOCCityGenerator> City;
	UPROPERTY() TObjectPtr<AOCWorldEnvironment> Environment;
	FTimerHandle RespawnTimer;
};
