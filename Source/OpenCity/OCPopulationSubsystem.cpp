#include "OCPopulationSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "OCCityGenerator.h"
#include "OCGameMode.h"
#include "OCPedestrian.h"
#include "OCVehicle.h"

TStatId UOCPopulationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOCPopulationSubsystem, STATGROUP_Tickables);
}

bool UOCPopulationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UOCPopulationSubsystem::RegisterNPC(AOCNPCCharacter* NPC)
{
	NPCs.AddUnique(NPC);
}

void UOCPopulationSubsystem::UnregisterNPC(AOCNPCCharacter* NPC)
{
	NPCs.RemoveSingleSwap(NPC);
}

void UOCPopulationSubsystem::BroadcastThreat(const FVector& Location, float Radius)
{
	const float RadiusSq = Radius * Radius;
	// Copy: reactions can spawn or destroy actors, which mutates the registry.
	const TArray<TWeakObjectPtr<AOCNPCCharacter>> Snapshot = NPCs;
	for (const TWeakObjectPtr<AOCNPCCharacter>& Weak : Snapshot)
	{
		AOCNPCCharacter* NPC = Weak.Get();
		if (NPC && !NPC->IsDead() && FVector::DistSquared(NPC->GetActorLocation(), Location) < RadiusSq)
		{
			NPC->ReactToThreat(Location);
		}
	}
}

int32 UOCPopulationSubsystem::CountWitnesses(const FVector& Location, float Radius) const
{
	const float RadiusSq = Radius * Radius;
	int32 Count = 0;
	for (const TWeakObjectPtr<AOCNPCCharacter>& Weak : NPCs)
	{
		const AOCNPCCharacter* NPC = Weak.Get();
		if (NPC && NPC->IsWitness() && FVector::DistSquared(NPC->GetActorLocation(), Location) < RadiusSq)
		{
			++Count;
		}
	}
	return Count;
}

void UOCPopulationSubsystem::Tick(float DeltaTime)
{
	UpdateTimer -= DeltaTime;
	if (UpdateTimer > 0.f)
	{
		return;
	}
	UpdateTimer = 0.5f;

	const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player)
	{
		return;
	}
	const FVector PlayerLocation = Player->GetActorLocation();
	UpdatePedestrians(PlayerLocation);
	UpdateTraffic(PlayerLocation);
}

void UOCPopulationSubsystem::UpdatePedestrians(const FVector& PlayerLocation)
{
	AOCGameMode* GM = AOCGameMode::Get(GetWorld());
	AOCCityGenerator* City = GM ? GM->GetCity() : nullptr;
	if (!City)
	{
		return;
	}

	NPCs.RemoveAll([](const TWeakObjectPtr<AOCNPCCharacter>& Weak) { return !Weak.IsValid(); });

	int32 Pedestrians = 0;
	TArray<AActor*> ToDestroy;
	const float DespawnSq = DespawnDistance * DespawnDistance;
	for (const TWeakObjectPtr<AOCNPCCharacter>& Weak : NPCs)
	{
		AOCPedestrian* Ped = Cast<AOCPedestrian>(Weak.Get());
		if (!Ped || Ped->IsDead())
		{
			continue;
		}
		if (FVector::DistSquared(Ped->GetActorLocation(), PlayerLocation) > DespawnSq)
		{
			ToDestroy.Add(Ped);
			continue;
		}
		++Pedestrians;
	}
	for (AActor* Actor : ToDestroy)
	{
		Actor->Destroy();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	for (int32 i = 0; i < 3 && Pedestrians < MaxPedestrians; ++i)
	{
		FIntPoint Block;
		int32 Corner = 0;
		if (!City->FindSidewalkSpawn(PlayerLocation, SpawnMinDistance, SpawnMaxDistance, Block, Corner))
		{
			continue;
		}
		const FVector Location = City->GetSidewalkCorner(Block, Corner);
		if (AOCPedestrian* Ped = GetWorld()->SpawnActor<AOCPedestrian>(AOCPedestrian::StaticClass(), Location, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), SpawnParams))
		{
			Ped->InitRoute(Block, (Corner + 1) % 4);
			++Pedestrians;
		}
	}
}

void UOCPopulationSubsystem::UpdateTraffic(const FVector& PlayerLocation)
{
	AOCGameMode* GM = AOCGameMode::Get(GetWorld());
	AOCCityGenerator* City = GM ? GM->GetCity() : nullptr;
	if (!City)
	{
		return;
	}

	Traffic.RemoveAll([](const TWeakObjectPtr<AOCVehicle>& Weak) { return !Weak.IsValid() || Weak->GetMode() != EOCVehicleMode::AI; });

	const float DespawnSq = DespawnDistance * DespawnDistance;
	TArray<AActor*> ToDestroy;
	for (const TWeakObjectPtr<AOCVehicle>& Weak : Traffic)
	{
		if (FVector::DistSquared(Weak->GetActorLocation(), PlayerLocation) > DespawnSq)
		{
			ToDestroy.Add(Weak.Get());
		}
	}
	for (AActor* Actor : ToDestroy)
	{
		Actor->Destroy();
	}
	Traffic.RemoveAll([](const TWeakObjectPtr<AOCVehicle>& Weak) { return !Weak.IsValid() || Weak->IsActorBeingDestroyed(); });

	if (Traffic.Num() >= MaxTraffic)
	{
		return;
	}

	FTransform SpawnTransform;
	FIntPoint NextIntersection;
	FIntPoint Direction;
	if (!City->FindLaneSpawn(PlayerLocation, SpawnMinDistance + 1000.f, SpawnMaxDistance, SpawnTransform, NextIntersection, Direction))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	if (AOCVehicle* Car = GetWorld()->SpawnActor<AOCVehicle>(AOCVehicle::StaticClass(), SpawnTransform, SpawnParams))
	{
		Car->StartAIDriving(NextIntersection, Direction);
		Traffic.Add(Car);
	}
}
