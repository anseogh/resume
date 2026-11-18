#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OCWantedSubsystem.generated.h"

class AOCPoliceOfficer;

/**
 * GTA-style wanted level. Witnessed crimes add heat, heat maps to 0-5 stars, police
 * spawn around the player proportional to stars, and staying out of police sight long
 * enough clears the level.
 */
UCLASS()
class OPENCITY_API UOCWantedSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	int32 GetStars() const { return Stars; }
	float GetHeat() const { return Heat; }
	/** True while wanted but no officer currently sees the player (stars blink on the HUD). */
	bool IsSearching() const;
	/** 0..1 progress towards losing the cops. */
	float GetEvadeProgress() const;
	FVector GetLastKnownLocation() const { return LastKnownLocation; }

	void ReportCrime(float HeatAmount, const FVector& Location, bool bRequireWitness = true);
	void NotifyPlayerSpotted(const FVector& Location);
	void ForceStars(int32 MinStars);
	void ClearWanted();

	int32 MaxPolice = 12;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void UpdateStars();
	void ManagePolice();
	float GetEvadeTime() const { return 10.f + Stars * 5.f; }
	float Now() const;

	float Heat = 0.f;
	int32 Stars = 0;
	float LastSpottedTime = -1000.f;
	FVector LastKnownLocation = FVector::ZeroVector;
	float ManageTimer = 0.f;
	TArray<TWeakObjectPtr<AOCPoliceOfficer>> Police;
};
