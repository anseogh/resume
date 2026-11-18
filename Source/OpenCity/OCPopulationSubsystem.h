#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OCPopulationSubsystem.generated.h"

class AOCNPCCharacter;
class AOCVehicle;

/**
 * Streams pedestrians and AI traffic in a ring around the player and despawns them when
 * far away, so only a bounded number of agents ever exist. Also the NPC registry used
 * for threat broadcasts, witnesses and minimap blips.
 */
UCLASS()
class OPENCITY_API UOCPopulationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	void RegisterNPC(AOCNPCCharacter* NPC);
	void UnregisterNPC(AOCNPCCharacter* NPC);

	void BroadcastThreat(const FVector& Location, float Radius);
	int32 CountWitnesses(const FVector& Location, float Radius) const;
	const TArray<TWeakObjectPtr<AOCNPCCharacter>>& GetNPCs() const { return NPCs; }

	int32 MaxPedestrians = 36;
	int32 MaxTraffic = 12;
	float SpawnMinDistance = 2500.f;
	float SpawnMaxDistance = 9000.f;
	float DespawnDistance = 12000.f;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void UpdatePedestrians(const FVector& PlayerLocation);
	void UpdateTraffic(const FVector& PlayerLocation);

	TArray<TWeakObjectPtr<AOCNPCCharacter>> NPCs;
	TArray<TWeakObjectPtr<AOCVehicle>> Traffic;
	float UpdateTimer = 0.f;
};
