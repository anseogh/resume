#include "OCPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "OCGameMode.h"
#include "OCStorySubsystem.h"
#include "OCWantedSubsystem.h"
#include "OCWorldEnvironment.h"

namespace
{
	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		return Action;
	}
}

void AOCPlayerController::EnsureInput()
{
	if (DefaultContext)
	{
		return;
	}

	DefaultContext = NewObject<UInputMappingContext>(this, TEXT("IMC_OpenCity"));
	MoveAction = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(this, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	FireAction = MakeAction(this, TEXT("IA_Fire"), EInputActionValueType::Boolean);
	AimAction = MakeAction(this, TEXT("IA_Aim"), EInputActionValueType::Boolean);
	HandbrakeAction = MakeAction(this, TEXT("IA_Handbrake"), EInputActionValueType::Boolean);

	UInputMappingContext* Context = DefaultContext;
	auto Map = [Context](UInputAction* Action, const FKey& Key, bool bSwizzle = false, bool bNegateX = false, bool bNegateY = false, bool bDeadZone = false)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bDeadZone)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierDeadZone>(Context));
		}
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegateX || bNegateY)
		{
			UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Context);
			Negate->bX = bNegateX;
			Negate->bY = bNegateY;
			Negate->bZ = false;
			Mapping.Modifiers.Add(Negate);
		}
	};

	// Move: X = right, Y = forward (also throttle/steer in vehicles).
	Map(MoveAction, EKeys::W, true);
	Map(MoveAction, EKeys::S, true, false, true);
	Map(MoveAction, EKeys::A, false, true);
	Map(MoveAction, EKeys::D);
	Map(MoveAction, EKeys::Up, true);
	Map(MoveAction, EKeys::Down, true, false, true);
	Map(MoveAction, EKeys::Left, false, true);
	Map(MoveAction, EKeys::Right);
	Map(MoveAction, EKeys::Gamepad_Left2D, false, false, false, true);

	Map(LookAction, EKeys::Mouse2D, false, false, true);
	Map(LookAction, EKeys::Gamepad_Right2D, false, false, true, true);

	Map(JumpAction, EKeys::SpaceBar);
	Map(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	Map(SprintAction, EKeys::LeftShift);
	Map(SprintAction, EKeys::Gamepad_LeftThumbstick);

	Map(InteractAction, EKeys::F);
	Map(InteractAction, EKeys::Gamepad_FaceButton_Top);

	Map(FireAction, EKeys::LeftMouseButton);
	Map(FireAction, EKeys::Gamepad_RightTrigger);

	Map(AimAction, EKeys::RightMouseButton);
	Map(AimAction, EKeys::Gamepad_LeftTrigger);

	Map(HandbrakeAction, EKeys::SpaceBar);
	Map(HandbrakeAction, EKeys::Gamepad_FaceButton_Right);
}

void AOCPlayerController::AddMappingContext()
{
	EnsureInput();
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(DefaultContext, 0);
		}
	}
}

void AOCPlayerController::BeginPlay()
{
	Super::BeginPlay();
	AddMappingContext();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AOCPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AddMappingContext();
}

void AOCPlayerController::OCResetStory()
{
	if (UOCStorySubsystem* Story = GetWorld()->GetSubsystem<UOCStorySubsystem>())
	{
		Story->ResetProgress();
	}
}

void AOCPlayerController::OCSkipObjective()
{
	if (UOCStorySubsystem* Story = GetWorld()->GetSubsystem<UOCStorySubsystem>())
	{
		Story->SkipObjective();
	}
}

void AOCPlayerController::OCWanted(int32 Stars)
{
	if (UOCWantedSubsystem* Wanted = GetWorld()->GetSubsystem<UOCWantedSubsystem>())
	{
		if (Stars <= 0)
		{
			Wanted->ClearWanted();
		}
		else
		{
			Wanted->ForceStars(Stars);
		}
	}
}

void AOCPlayerController::OCSetTime(float Hour)
{
	if (AOCGameMode* GM = AOCGameMode::Get(this))
	{
		if (AOCWorldEnvironment* Env = GM->GetEnvironment())
		{
			Env->SetTimeOfDay(Hour);
		}
	}
}
