#include "OCWantedSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "OCCityGenerator.h"
#include "OCGameMode.h"
#include "OCHUD.h"
#include "OCPoliceOfficer.h"
#include "OCPopulationSubsystem.h"
#include "OCText.h"

namespace
{
	const float StarThresholds[] = { 1.f, 4.f, 9.f, 15.f, 22.f };
}

TStatId UOCWantedSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOCWantedSubsystem, STATGROUP_Tickables);
}

bool UOCWantedSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

float UOCWantedSubsystem::Now() const
{
	return GetWorld()->GetTimeSeconds();
}

bool UOCWantedSubsystem::IsSearching() const
{
	return Stars > 0 && Now() - LastSpottedTime > 3.f;
}

float UOCWantedSubsystem::GetEvadeProgress() const
{
	return Stars > 0 ? FMath::Clamp((Now() - LastSpottedTime) / GetEvadeTime(), 0.f, 1.f) : 0.f;
}

void UOCWantedSubsystem::ReportCrime(float HeatAmount, const FVector& Location, bool bRequireWitness)
{
	if (HeatAmount <= 0.f)
	{
		return;
	}

	if (bRequireWitness)
	{
		const UOCPopulationSubsystem* Population = GetWorld()->GetSubsystem<UOCPopulationSubsystem>();
		if (!Population || Population->CountWitnesses(Location, 3500.f) == 0)
		{
			return;
		}
	}

	Heat = FMath::Min(Heat + HeatAmount, 30.f);
	LastKnownLocation = Location;
	LastSpottedTime = Now();

	const int32 OldStars = Stars;
	UpdateStars();
	if (Stars > OldStars)
	{
		AOCHUD::ShowNotification(GetWorld(), OCText::WantedUp, 2.f);
	}
}

void UOCWantedSubsystem::NotifyPlayerSpotted(const FVector& Location)
{
	if (Stars == 0)
	{
		return;
	}
	LastSpottedTime = Now();
	LastKnownLocation = Location;
}

void UOCWantedSubsystem::ForceStars(int32 MinStars)
{
	MinStars = FMath::Clamp(MinStars, 0, 5);
	if (MinStars > 0)
	{
		Heat = FMath::Max(Heat, StarThresholds[MinStars - 1]);
		LastSpottedTime = Now();
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			LastKnownLocation = Player->GetActorLocation();
		}
	}
	UpdateStars();
}

void UOCWantedSubsystem::ClearWanted()
{
	Heat = 0.f;
	Stars = 0;
}

void UOCWantedSubsystem::UpdateStars()
{
	int32 NewStars = 0;
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(StarThresholds)); ++i)
	{
		if (Heat >= StarThresholds[i])
		{
			NewStars = i + 1;
		}
	}
	Stars = FMath::Max(Stars, NewStars);
}

void UOCWantedSubsystem::Tick(float DeltaTime)
{
	if (Stars > 0 && Now() - LastSpottedTime > GetEvadeTime())
	{
		ClearWanted();
		AOCHUD::ShowNotification(GetWorld(), OCText::LostCops, 3.f);
	}

	ManageTimer -= DeltaTime;
	if (ManageTimer <= 0.f)
	{
		ManageTimer = 1.f;
		ManagePolice();
	}
}

void UOCWantedSubsystem::ManagePolice()
{
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	AOCGameMode* GM = AOCGameMode::Get(World);
	AOCCityGenerator* City = GM ? GM->GetCity() : nullptr;
	if (!Player || !City)
	{
		return;
	}
	const FVector PlayerLocation = Player->GetActorLocation();

	Police.RemoveAll([](const TWeakObjectPtr<AOCPoliceOfficer>& Weak) { return !Weak.IsValid() || Weak->IsDead(); });

	const float DespawnDistance = Stars == 0 ? 5000.f : 15000.f;
	TArray<AActor*> ToDestroy;
	for (const TWeakObjectPtr<AOCPoliceOfficer>& Weak : Police)
	{
		if (FVector::Dist(Weak->GetActorLocation(), PlayerLocation) > DespawnDistance)
		{
			ToDestroy.Add(Weak.Get());
		}
	}
	for (AActor* Actor : ToDestroy)
	{
		Actor->Destroy();
	}
	Police.RemoveAll([](const TWeakObjectPtr<AOCPoliceOfficer>& Weak) { return !Weak.IsValid() || Weak->IsActorBeingDestroyed(); });

	const int32 Desired = Stars == 0 ? 0 : FMath::Min(MaxPolice, Stars * 2 + 1);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	for (int32 i = 0; i < 2 && Police.Num() < Desired; ++i)
	{
		FIntPoint Block;
		int32 Corner = 0;
		if (!City->FindSidewalkSpawn(LastKnownLocation, 2500.f, 6000.f, Block, Corner))
		{
			continue;
		}
		const FVector Location = City->GetSidewalkCorner(Block, Corner);
		if (AOCPoliceOfficer* Officer = World->SpawnActor<AOCPoliceOfficer>(AOCPoliceOfficer::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams))
		{
			Police.Add(Officer);
		}
	}
}
