#pragma once

#include "CoreMinimal.h"
#include "OCNPCCharacter.h"
#include "OCPedestrian.generated.h"

/** Civilian that walks around block sidewalks, crosses at corners and panics at violence. */
UCLASS()
class OPENCITY_API AOCPedestrian : public AOCNPCCharacter
{
	GENERATED_BODY()

public:
	AOCPedestrian();

	void InitRoute(FIntPoint InBlock, int32 InCorner);
	virtual void ReactToThreat(const FVector& ThreatLocation) override;
	virtual bool IsWitness() const override { return !IsDead(); }
	bool IsPanicking() const { return PanicTime > 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void TickAI(float DeltaSeconds) override;
	virtual float GetCrimeHeatForDamage() const override { return 0.5f; }
	virtual float GetCrimeHeatForKill() const override { return 1.5f; }
	virtual FLinearColor GetClothingColor() const override { return Clothing; }

private:
	void EnsureRoute();
	void AdvanceRoute();

	FIntPoint Block = FIntPoint(-1, -1);
	int32 Corner = 0;
	bool bClockwise = true;
	float PauseTime = 0.f;
	float PanicTime = 0.f;
	float WalkSpeed = 150.f;
	FVector ThreatOrigin = FVector::ZeroVector;
	FLinearColor Clothing = FLinearColor::Gray;
};
