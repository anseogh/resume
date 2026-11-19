#pragma once

#include "CoreMinimal.h"
#include "OCNPCCharacter.h"
#include "OCPoliceOfficer.generated.h"

/** Spawned by the wanted system. Hunts the player's last known position and opens fire on sight. */
UCLASS()
class OPENCITY_API AOCPoliceOfficer : public AOCNPCCharacter
{
	GENERATED_BODY()

public:
	AOCPoliceOfficer();

	virtual FLinearColor GetBlipColor() const override;
	virtual bool IsWitness() const override { return !IsDead(); }
	bool CanSeePlayer() const { return bCanSeePlayer; }

protected:
	virtual void BeginPlay() override;
	virtual void TickAI(float DeltaSeconds) override;
	virtual float GetCrimeHeatForDamage() const override { return 1.f; }
	virtual float GetCrimeHeatForKill() const override { return 3.f; }
	virtual FLinearColor GetClothingColor() const override { return FLinearColor(0.02f, 0.04f, 0.15f); }

private:
	float SightTimer = 0.f;
	float ShootCooldown = 1.f;
	bool bCanSeePlayer = false;
};
