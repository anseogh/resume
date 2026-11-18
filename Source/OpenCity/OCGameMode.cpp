#include "OCGameMode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "OCCharacter.h"
#include "OCCityGenerator.h"
#include "OCHUD.h"
#include "OCPlayerController.h"
#include "OCStorySubsystem.h"
#include "OCText.h"
#include "OCWantedSubsystem.h"
#include "OCWorldEnvironment.h"

AOCGameMode::AOCGameMode()
{
	DefaultPawnClass = AOCCharacter::StaticClass();
	PlayerControllerClass = AOCPlayerController::StaticClass();
	HUDClass = AOCHUD::StaticClass();
}

AOCGameMode* AOCGameMode::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetAuthGameMode<AOCGameMode>() : nullptr;
}

void AOCGameMode::EnsureWorld()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!City)
	{
		// A level may already contain a hand-placed, tweaked generator.
		for (TActorIterator<AOCCityGenerator> It(World); It; ++It)
		{
			City = *It;
			break;
		}
		if (!City)
		{
			City = World->SpawnActor<AOCCityGenerator>(AOCCityGenerator::StaticClass(), FTransform::Identity, SpawnParams);
		}
		City->Generate();
	}

	if (!Environment)
	{
		for (TActorIterator<AOCWorldEnvironment> It(World); It; ++It)
		{
			Environment = *It;
			break;
		}
		if (!Environment)
		{
			Environment = World->SpawnActor<AOCWorldEnvironment>(AOCWorldEnvironment::StaticClass(), FTransform::Identity, SpawnParams);
		}
	}
}

void AOCGameMode::StartPlay()
{
	EnsureWorld();
	Super::StartPlay();
}

void AOCGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	EnsureWorld();
	RestartPlayerAtTransform(NewPlayer, City->GetPlayerSpawnTransform());
}

void AOCGameMode::HandlePlayerDeath(APlayerController* PC)
{
	UWorld* World = GetWorld();
	if (UOCWantedSubsystem* Wanted = World->GetSubsystem<UOCWantedSubsystem>())
	{
		Wanted->ClearWanted();
	}
	if (UOCStorySubsystem* Story = World->GetSubsystem<UOCStorySubsystem>())
	{
		Story->OnPlayerDied();
	}
	AOCHUD::ShowBanner(this, OCText::Wasted, OCText::WastedSub, FLinearColor(0.85f, 0.05f, 0.05f), 5.f);

	GetWorldTimerManager().SetTimer(RespawnTimer,
		FTimerDelegate::CreateUObject(this, &AOCGameMode::RespawnPlayer, TWeakObjectPtr<APlayerController>(PC)), 5.f, false);
}

void AOCGameMode::RespawnPlayer(TWeakObjectPtr<APlayerController> PC)
{
	if (!PC.IsValid())
	{
		return;
	}
	APawn* OldPawn = PC->GetPawn();
	PC->UnPossess();
	if (OldPawn)
	{
		OldPawn->SetLifeSpan(10.f);
	}
	RestartPlayer(PC.Get());
}
