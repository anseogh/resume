#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OCStorySubsystem.generated.h"

class AOCCityGenerator;
class AOCGangMember;
class AOCMarker;

enum class EOCObjectiveType : uint8
{
	GoTo,
	EnterVehicle,
	DriveTo,
	Eliminate,
	LoseWanted,
	Survive,
	GetWanted
};

struct FOCDialogueLine
{
	FString Speaker;
	FString Text;
	float Duration = 4.f;
};

struct FOCObjective
{
	EOCObjectiveType Type = EOCObjectiveType::GoTo;
	FString Text;
	FIntPoint At = FIntPoint::ZeroValue;
	float Radius = 600.f;
	int32 Count = 0;
	float Seconds = 0.f;
	int32 Stars = 0;
};

struct FOCMission
{
	FString Id;
	FString Title;
	FIntPoint Start = FIntPoint::ZeroValue;
	TArray<FOCDialogueLine> Intro;
	TArray<FOCDialogueLine> Outro;
	TArray<FOCObjective> Objectives;
	int32 Reward = 0;
};

/**
 * Data-driven story. Missions are read from Content/Story/Story.json, so writers can add
 * or rewrite missions without touching C++ or the editor. Progress and money are saved.
 */
UCLASS()
class OPENCITY_API UOCStorySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	bool IsMissionActive() const { return ActiveMission != INDEX_NONE; }
	FString GetMissionTitle() const;
	FString GetObjectiveText() const;
	bool GetCurrentDialogue(FString& OutSpeaker, FString& OutText) const;
	bool GetObjectiveLocation(FVector& OutLocation) const;
	const FString& GetStoryTitle() const { return StoryTitle; }

	int32 GetMoney() const { return Money; }
	void AddMoney(int32 Amount);
	int32 GetCompletedMissions() const { return Completed; }
	int32 GetMissionCount() const { return Missions.Num(); }

	void OnPlayerDied();
	void SkipObjective();
	void ResetProgress();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void LoadStory();
	void LoadProgress();
	void SaveProgress();

	void StartMission(int32 Index);
	void BeginObjective();
	void CompleteObjective();
	void CompleteMission();
	void FailMission(const FString& Reason);
	void CleanupMission();
	bool IsObjectiveComplete(float DeltaTime);
	int32 CountAliveEnemies() const;

	void QueueDialogue(const TArray<FOCDialogueLine>& Lines);
	void ShowMarker(const FVector& Location, const FLinearColor& Color);
	void HideMarker();
	AOCCityGenerator* GetCity() const;

	FString StoryTitle;
	TArray<FOCMission> Missions;
	bool bStoryLoaded = false;

	int32 ActiveMission = INDEX_NONE;
	int32 ObjectiveIndex = 0;
	float ObjectiveTimer = 0.f;
	float IdleCooldown = 2.f;
	bool bAnnouncedEnding = false;

	TArray<FOCDialogueLine> DialogueQueue;
	float DialogueTimer = 0.f;

	TWeakObjectPtr<AOCMarker> Marker;
	TArray<TWeakObjectPtr<AOCGangMember>> Enemies;

	int32 Money = 0;
	int32 Completed = 0;
};
