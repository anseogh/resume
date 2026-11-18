#pragma once

#include "CoreMinimal.h"
#include "OCNPCCharacter.h"
#include "OCGangMember.generated.h"

/** Mission enemy. Guards a spot until it sees the player or gets shot, then fights. */
UCLASS()
class OPENCITY_API AOCGangMember : public AOCNPCCharacter
{
	GENERATED_BODY()

public:
	AOCGangMember();

	virtual void ReactToThreat(const FVector& ThreatLocation) override;
	virtual FLinearColor GetBlipColor() const override;

protected:
	virtual void TickAI(float DeltaSeconds) override;
	virtual float GetCrimeHeatForKill() const override { return 0.4f; }
	virtual FLinearColor GetClothingColor() const override { return FLinearColor(0.45f, 0.02f, 0.02f); }

private:
	bool bAggro = false;
	bool bCanSeePlayer = false;
	float SightTimer = 0.f;
	float ShootCooldown = 1.f;
	float StrafeSign = 1.f;
	float StrafeTimer = 0.f;
};
