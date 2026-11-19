#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCMarker.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/** Mission marker: ground ring, a tall beam visible across the city, and a light. */
UCLASS()
class OPENCITY_API AOCMarker : public AActor
{
	GENERATED_BODY()

public:
	AOCMarker();

	virtual void Tick(float DeltaSeconds) override;
	void SetColor(const FLinearColor& Color);

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ring;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Beam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;

	float Time = 0.f;
};
