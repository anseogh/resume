#include "OCStorySubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OCCityGenerator.h"
#include "OCGameMode.h"
#include "OCGangMember.h"
#include "OCHUD.h"
#include "OCMarker.h"
#include "OCSaveGame.h"
#include "OCText.h"
#include "OCVehicle.h"
#include "OCWantedSubsystem.h"
#include "OpenCity.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* SaveSlot = TEXT("OpenCity");

	EOCObjectiveType ParseObjectiveType(const FString& Type)
	{
		if (Type == TEXT("enter_vehicle")) return EOCObjectiveType::EnterVehicle;
		if (Type == TEXT("drive_to")) return EOCObjectiveType::DriveTo;
		if (Type == TEXT("eliminate")) return EOCObjectiveType::Eliminate;
		if (Type == TEXT("lose_wanted")) return EOCObjectiveType::LoseWanted;
		if (Type == TEXT("survive")) return EOCObjectiveType::Survive;
		if (Type == TEXT("get_wanted")) return EOCObjectiveType::GetWanted;
		return EOCObjectiveType::GoTo;
	}

	const TCHAR* DefaultObjectiveText(EOCObjectiveType Type)
	{
		switch (Type)
		{
		case EOCObjectiveType::EnterVehicle: return OCText::ObjEnterVehicle;
		case EOCObjectiveType::DriveTo: return OCText::ObjDriveTo;
		case EOCObjectiveType::Eliminate: return OCText::ObjEliminate;
		case EOCObjectiveType::LoseWanted: return OCText::ObjLoseWanted;
		case EOCObjectiveType::Survive: return OCText::ObjSurvive;
		case EOCObjectiveType::GetWanted: return OCText::ObjGetWanted;
		default: return OCText::ObjGoTo;
		}
	}

	FIntPoint ReadPoint(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Object->TryGetArrayField(Field, Values) && Values->Num() >= 2)
		{
			return FIntPoint(static_cast<int32>((*Values)[0]->AsNumber()), static_cast<int32>((*Values)[1]->AsNumber()));
		}
		return FIntPoint::ZeroValue;
	}

	TArray<FOCDialogueLine> ReadDialogue(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		TArray<FOCDialogueLine> Lines;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object->TryGetArrayField(Field, Values))
		{
			return Lines;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			const TSharedPtr<FJsonObject> LineObject = Value->AsObject();
			if (!LineObject.IsValid())
			{
				continue;
			}
			FOCDialogueLine Line;
			LineObject->TryGetStringField(TEXT("speaker"), Line.Speaker);
			LineObject->TryGetStringField(TEXT("text"), Line.Text);
			double Duration = 0.0;
			Line.Duration = LineObject->TryGetNumberField(TEXT("duration"), Duration)
				? static_cast<float>(Duration)
				: 2.f + Line.Text.Len() * 0.07f;
			Lines.Add(Line);
		}
		return Lines;
	}
}

TStatId UOCStorySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOCStorySubsystem, STATGROUP_Tickables);
}

bool UOCStorySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UOCStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadStory();
	LoadProgress();
}

void UOCStorySubsystem::LoadStory()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Story/Story.json");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogOpenCity, Warning, TEXT("Story file not found: %s"), *Path);
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogOpenCity, Error, TEXT("Story file is not valid JSON: %s"), *Path);
		return;
	}

	Root->TryGetStringField(TEXT("title"), StoryTitle);
	const TArray<TSharedPtr<FJsonValue>>* MissionValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("missions"), MissionValues))
	{
		return;
	}

	for (const TSharedPtr<FJsonValue>& MissionValue : *MissionValues)
	{
		const TSharedPtr<FJsonObject> MissionObject = MissionValue->AsObject();
		if (!MissionObject.IsValid())
		{
			continue;
		}

		FOCMission Mission;
		MissionObject->TryGetStringField(TEXT("id"), Mission.Id);
		MissionObject->TryGetStringField(TEXT("title"), Mission.Title);
		Mission.Start = ReadPoint(MissionObject, TEXT("start"));
		Mission.Intro = ReadDialogue(MissionObject, TEXT("intro"));
		Mission.Outro = ReadDialogue(MissionObject, TEXT("outro"));
		double Reward = 0.0;
		if (MissionObject->TryGetNumberField(TEXT("reward"), Reward))
		{
			Mission.Reward = static_cast<int32>(Reward);
		}

		const TArray<TSharedPtr<FJsonValue>>* ObjectiveValues = nullptr;
		if (MissionObject->TryGetArrayField(TEXT("objectives"), ObjectiveValues))
		{
			for (const TSharedPtr<FJsonValue>& ObjectiveValue : *ObjectiveValues)
			{
				const TSharedPtr<FJsonObject> ObjectiveObject = ObjectiveValue->AsObject();
				if (!ObjectiveObject.IsValid())
				{
					continue;
				}
				FOCObjective Objective;
				FString Type;
				ObjectiveObject->TryGetStringField(TEXT("type"), Type);
				Objective.Type = ParseObjectiveType(Type);
				if (!ObjectiveObject->TryGetStringField(TEXT("text"), Objective.Text))
				{
					Objective.Text = DefaultObjectiveText(Objective.Type);
				}
				Objective.At = ReadPoint(ObjectiveObject, TEXT("at"));
				double Number = 0.0;
				if (ObjectiveObject->TryGetNumberField(TEXT("radius"), Number)) Objective.Radius = static_cast<float>(Number);
				if (ObjectiveObject->TryGetNumberField(TEXT("count"), Number)) Objective.Count = static_cast<int32>(Number);
				if (ObjectiveObject->TryGetNumberField(TEXT("seconds"), Number)) Objective.Seconds = static_cast<float>(Number);
				if (ObjectiveObject->TryGetNumberField(TEXT("stars"), Number)) Objective.Stars = static_cast<int32>(Number);
				Mission.Objectives.Add(Objective);
			}
		}
		Missions.Add(Mission);
	}

	bStoryLoaded = true;
	UE_LOG(LogOpenCity, Log, TEXT("Loaded story '%s' with %d missions"), *StoryTitle, Missions.Num());
}

void UOCStorySubsystem::LoadProgress()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlot, 0))
	{
		return;
	}
	if (const UOCSaveGame* Save = Cast<UOCSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0)))
	{
		Completed = FMath::Clamp(Save->CompletedMissions, 0, Missions.Num());
		Money = FMath::Max(0, Save->Money);
	}
}

void UOCStorySubsystem::SaveProgress()
{
	UOCSaveGame* Save = Cast<UOCSaveGame>(UGameplayStatics::CreateSaveGameObject(UOCSaveGame::StaticClass()));
	if (Save)
	{
		Save->CompletedMissions = Completed;
		Save->Money = Money;
		UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
	}
}

void UOCStorySubsystem::ResetProgress()
{
	CleanupMission();
	ActiveMission = INDEX_NONE;
	Completed = 0;
	Money = 0;
	bAnnouncedEnding = false;
	DialogueQueue.Reset();
	SaveProgress();
}

void UOCStorySubsystem::AddMoney(int32 Amount)
{
	Money = FMath::Max(0, Money + Amount);
}

AOCCityGenerator* UOCStorySubsystem::GetCity() const
{
	AOCGameMode* GM = AOCGameMode::Get(GetWorld());
	return GM ? GM->GetCity() : nullptr;
}

void UOCStorySubsystem::Tick(float DeltaTime)
{
	if (DialogueQueue.Num() > 0)
	{
		DialogueTimer += DeltaTime;
		if (DialogueTimer >= DialogueQueue[0].Duration)
		{
			DialogueQueue.RemoveAt(0);
			DialogueTimer = 0.f;
		}
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	AOCCityGenerator* City = GetCity();
	if (!Player || !City || !bStoryLoaded)
	{
		return;
	}

	if (ActiveMission == INDEX_NONE)
	{
		if (IdleCooldown > 0.f)
		{
			IdleCooldown -= DeltaTime;
			return;
		}
		if (Completed >= Missions.Num())
		{
			HideMarker();
			if (!bAnnouncedEnding && Missions.Num() > 0)
			{
				bAnnouncedEnding = true;
				AOCHUD::ShowBanner(GetWorld(), StoryTitle, OCText::StoryComplete, FLinearColor(1.f, 0.8f, 0.2f), 6.f);
			}
			return;
		}

		const FVector Start = City->GetIntersectionLocation(Missions[Completed].Start);
		ShowMarker(Start, FLinearColor(1.f, 0.8f, 0.05f));
		if (FVector::Dist2D(Player->GetActorLocation(), Start) < 450.f)
		{
			StartMission(Completed);
		}
		return;
	}

	if (IsObjectiveComplete(DeltaTime))
	{
		CompleteObjective();
	}
}

void UOCStorySubsystem::StartMission(int32 Index)
{
	ActiveMission = Index;
	ObjectiveIndex = 0;
	const FOCMission& Mission = Missions[Index];
	AOCHUD::ShowBanner(GetWorld(), Mission.Title, StoryTitle, FLinearColor(1.f, 0.85f, 0.3f), 3.5f);
	QueueDialogue(Mission.Intro);
	BeginObjective();
}

void UOCStorySubsystem::BeginObjective()
{
	const FOCMission& Mission = Missions[ActiveMission];
	if (!Mission.Objectives.IsValidIndex(ObjectiveIndex))
	{
		CompleteMission();
		return;
	}

	const FOCObjective& Objective = Mission.Objectives[ObjectiveIndex];
	ObjectiveTimer = 0.f;
	Enemies.Reset();
	AOCCityGenerator* City = GetCity();

	switch (Objective.Type)
	{
	case EOCObjectiveType::GoTo:
	case EOCObjectiveType::DriveTo:
		ShowMarker(City->GetIntersectionLocation(Objective.At), FLinearColor(1.f, 0.8f, 0.05f));
		break;

	case EOCObjectiveType::Eliminate:
	{
		HideMarker();
		const FVector Center = City->GetIntersectionLocation(Objective.At);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		for (int32 i = 0; i < FMath::Max(1, Objective.Count); ++i)
		{
			const FVector Offset(FMath::FRandRange(-650.f, 650.f), FMath::FRandRange(-650.f, 650.f), 110.f);
			if (AOCGangMember* Enemy = GetWorld()->SpawnActor<AOCGangMember>(AOCGangMember::StaticClass(), Center + Offset, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), SpawnParams))
			{
				Enemies.Add(Enemy);
			}
		}
		break;
	}

	default:
		HideMarker();
		break;
	}
}

int32 UOCStorySubsystem::CountAliveEnemies() const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<AOCGangMember>& Enemy : Enemies)
	{
		if (Enemy.IsValid() && !Enemy->IsDead())
		{
			++Alive;
		}
	}
	return Alive;
}

bool UOCStorySubsystem::IsObjectiveComplete(float DeltaTime)
{
	const FOCObjective& Objective = Missions[ActiveMission].Objectives[ObjectiveIndex];
	ObjectiveTimer += DeltaTime;

	const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	const UOCWantedSubsystem* Wanted = GetWorld()->GetSubsystem<UOCWantedSubsystem>();
	AOCCityGenerator* City = GetCity();
	const bool bInVehicle = Cast<AOCVehicle>(Player) != nullptr;
	const float Dist = City ? FVector::Dist2D(Player->GetActorLocation(), City->GetIntersectionLocation(Objective.At)) : 0.f;

	switch (Objective.Type)
	{
	case EOCObjectiveType::GoTo: return Dist < Objective.Radius;
	case EOCObjectiveType::EnterVehicle: return bInVehicle;
	case EOCObjectiveType::DriveTo: return bInVehicle && Dist < Objective.Radius;
	case EOCObjectiveType::Eliminate: return CountAliveEnemies() == 0;
	case EOCObjectiveType::LoseWanted: return Wanted && Wanted->GetStars() == 0;
	case EOCObjectiveType::Survive: return ObjectiveTimer >= Objective.Seconds;
	case EOCObjectiveType::GetWanted: return Wanted && Wanted->GetStars() >= FMath::Max(1, Objective.Stars);
	default: return true;
	}
}

void UOCStorySubsystem::CompleteObjective()
{
	++ObjectiveIndex;
	if (ObjectiveIndex >= Missions[ActiveMission].Objectives.Num())
	{
		CompleteMission();
		return;
	}
	AOCHUD::ShowNotification(GetWorld(), OCText::ObjectiveComplete, 2.f);
	BeginObjective();
}

void UOCStorySubsystem::CompleteMission()
{
	const FOCMission& Mission = Missions[ActiveMission];
	QueueDialogue(Mission.Outro);
	AddMoney(Mission.Reward);
	Completed = FMath::Max(Completed, ActiveMission + 1);
	AOCHUD::ShowBanner(GetWorld(), OCText::MissionPassed, FString::Printf(TEXT("+$%s"), *FString::FormatAsNumber(Mission.Reward)), FLinearColor(1.f, 0.8f, 0.2f), 5.f);

	CleanupMission();
	ActiveMission = INDEX_NONE;
	IdleCooldown = 8.f;
	SaveProgress();
}

void UOCStorySubsystem::FailMission(const FString& Reason)
{
	AOCHUD::ShowBanner(GetWorld(), OCText::MissionFailed, Reason, FLinearColor(0.9f, 0.15f, 0.1f), 4.f);
	CleanupMission();
	ActiveMission = INDEX_NONE;
	IdleCooldown = 6.f;
}

void UOCStorySubsystem::CleanupMission()
{
	HideMarker();
	for (const TWeakObjectPtr<AOCGangMember>& Enemy : Enemies)
	{
		if (Enemy.IsValid() && !Enemy->IsDead())
		{
			Enemy->Destroy();
		}
	}
	Enemies.Reset();
}

void UOCStorySubsystem::OnPlayerDied()
{
	AddMoney(-500);
	if (IsMissionActive())
	{
		DialogueQueue.Reset();
		FailMission(OCText::FailedDied);
	}
	SaveProgress();
}

void UOCStorySubsystem::SkipObjective()
{
	if (IsMissionActive())
	{
		CompleteObjective();
	}
	else if (Completed < Missions.Num())
	{
		StartMission(Completed);
	}
}

void UOCStorySubsystem::QueueDialogue(const TArray<FOCDialogueLine>& Lines)
{
	if (DialogueQueue.Num() == 0)
	{
		DialogueTimer = 0.f;
	}
	DialogueQueue.Append(Lines);
}

bool UOCStorySubsystem::GetCurrentDialogue(FString& OutSpeaker, FString& OutText) const
{
	if (DialogueQueue.Num() == 0)
	{
		return false;
	}
	OutSpeaker = DialogueQueue[0].Speaker;
	OutText = DialogueQueue[0].Text;
	return true;
}

FString UOCStorySubsystem::GetMissionTitle() const
{
	return IsMissionActive() ? Missions[ActiveMission].Title : FString();
}

FString UOCStorySubsystem::GetObjectiveText() const
{
	if (!bStoryLoaded)
	{
		return OCText::StoryMissing;
	}
	if (!IsMissionActive())
	{
		return Completed < Missions.Num() && IdleCooldown <= 0.f ? FString(OCText::GoToMissionStart) : FString();
	}

	const FOCObjective& Objective = Missions[ActiveMission].Objectives[ObjectiveIndex];
	switch (Objective.Type)
	{
	case EOCObjectiveType::Eliminate:
	{
		const int32 Total = Enemies.Num();
		return FString::Printf(TEXT("%s  (%d/%d)"), *Objective.Text, Total - CountAliveEnemies(), Total);
	}
	case EOCObjectiveType::Survive:
		return FString::Printf(TEXT("%s  %d%s"), *Objective.Text, FMath::CeilToInt(FMath::Max(0.f, Objective.Seconds - ObjectiveTimer)), OCText::Seconds);
	default:
		return Objective.Text;
	}
}

bool UOCStorySubsystem::GetObjectiveLocation(FVector& OutLocation) const
{
	const AOCCityGenerator* City = GetCity();
	if (!City || !bStoryLoaded)
	{
		return false;
	}

	if (!IsMissionActive())
	{
		if (Completed < Missions.Num() && IdleCooldown <= 0.f)
		{
			OutLocation = City->GetIntersectionLocation(Missions[Completed].Start);
			return true;
		}
		return false;
	}

	const FOCObjective& Objective = Missions[ActiveMission].Objectives[ObjectiveIndex];
	switch (Objective.Type)
	{
	case EOCObjectiveType::GoTo:
	case EOCObjectiveType::DriveTo:
		OutLocation = City->GetIntersectionLocation(Objective.At);
		return true;
	case EOCObjectiveType::Eliminate:
		for (const TWeakObjectPtr<AOCGangMember>& Enemy : Enemies)
		{
			if (Enemy.IsValid() && !Enemy->IsDead())
			{
				OutLocation = Enemy->GetActorLocation();
				return true;
			}
		}
		return false;
	default:
		return false;
	}
}

void UOCStorySubsystem::ShowMarker(const FVector& Location, const FLinearColor& Color)
{
	if (!Marker.IsValid())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Marker = GetWorld()->SpawnActor<AOCMarker>(AOCMarker::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
		if (Marker.IsValid())
		{
			Marker->SetColor(Color);
		}
		return;
	}
	if (!Marker->GetActorLocation().Equals(Location, 1.f))
	{
		Marker->SetActorLocation(Location);
		Marker->SetColor(Color);
	}
}

void UOCStorySubsystem::HideMarker()
{
	if (Marker.IsValid())
	{
		Marker->Destroy();
	}
	Marker = nullptr;
}
