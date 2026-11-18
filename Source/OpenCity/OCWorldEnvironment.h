#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCWorldEnvironment.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

/** Physically based sky + sun/moon with a running day-night cycle (Lumen lit, no baking). */
UCLASS()
class OPENCITY_API AOCWorldEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AOCWorldEnvironment();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Time") float TimeOfDay = 8.5f;
	/** Real-time minutes for one full in-game day. */
	UPROPERTY(EditAnywhere, Category = "Time") float DayLengthMinutes = 24.f;
	UPROPERTY(EditAnywhere, Category = "Time") bool bAdvanceTime = true;

	void SetTimeOfDay(float Hour);
	float GetTimeOfDay() const { return TimeOfDay; }
	bool IsNight() const { return TimeOfDay < 6.5f || TimeOfDay > 19.f; }

protected:
	virtual void BeginPlay() override;

private:
	void UpdateLighting();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> Fog;

	float LightingTimer = 0.f;
};
