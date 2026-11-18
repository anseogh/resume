#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OCPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Owns every input action. Actions and the mapping context are created in code so the
 * project needs no binary input assets; pawns bind to them in SetupPlayerInputComponent.
 */
UCLASS()
class OPENCITY_API AOCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void EnsureInput();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> DefaultContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> FireAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> AimAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> HandbrakeAction;

	// Developer console commands (open the console with ` and type e.g. "OCWanted 3").
	UFUNCTION(Exec) void OCResetStory();
	UFUNCTION(Exec) void OCSkipObjective();
	UFUNCTION(Exec) void OCWanted(int32 Stars);
	UFUNCTION(Exec) void OCSetTime(float Hour);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void AddMappingContext();
};
