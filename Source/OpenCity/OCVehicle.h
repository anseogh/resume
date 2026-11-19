#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "OCVehicle.generated.h"

class AOCCharacter;
class UBoxComponent;
class UCameraComponent;
class UPointLightComponent;
class USpotLightComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UENUM()
enum class EOCVehicleMode : uint8
{
	Parked,
	Player,
	AI,
	Wrecked
};

/**
 * Arcade raycast-suspension car. Needs no skeletal mesh or Chaos vehicle assets.
 * Parked cars far from the player turn physics off entirely ("dormant") to keep the
 * city cheap; the same class drives AI traffic along the road grid.
 */
UCLASS()
class OPENCITY_API AOCVehicle : public APawn
{
	GENERATED_BODY()

public:
	AOCVehicle();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	bool EnterVehicle(AOCCharacter* Character);
	void ExitVehicle();
	void StartAIDriving(FIntPoint NextIntersection, FIntPoint Direction);
	void SetBodyColor(const FLinearColor& Color);

	EOCVehicleMode GetMode() const { return Mode; }
	bool IsWrecked() const { return Mode == EOCVehicleMode::Wrecked; }
	bool IsBurning() const { return bBurning; }
	bool IsPlayerDriven() const { return Mode == EOCVehicleMode::Player; }
	AOCCharacter* GetDriver() const { return Driver.Get(); }
	float GetSpeedKmh() const { return GetVelocity().Size() * 0.036f; }
	float GetHealthFraction() const { return FMath::Clamp(Health / MaxHealth, 0.f, 1.f); }

	/** Height of the body origin above the ground when the suspension is at rest. */
	static float GetRestHeight() { return 88.f; }

	UPROPERTY(EditAnywhere, Category = "Vehicle|Suspension") float SuspensionRestLength = 45.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Suspension") float WheelRadius = 35.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Suspension") float SpringStrength = 650000.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Suspension") float DamperStrength = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float Mass = 1300.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float EngineForce = 1100000.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float BrakeForce = 2200000.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float MaxReverseSpeed = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float AirDrag = 0.036f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float RollingResistance = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Drive") float MaxSteerAngle = 34.f;

	UPROPERTY(EditAnywhere, Category = "Vehicle|Tires") float FrontGrip = 10.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Tires") float RearGrip = 9.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Tires") float HandbrakeGrip = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Tires") float FrictionCoefficient = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Vehicle|Damage") float MaxHealth = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Vehicle|Optimization") float WakeDistance = 6000.f;
	UPROPERTY(EditAnywhere, Category = "Vehicle|Optimization") float SleepDistance = 9000.f;

protected:
	virtual void BeginPlay() override;

private:
	void UpdatePhysics(float DeltaSeconds);
	void UpdateAI(float DeltaSeconds);
	void UpdateDormancy();
	void UpdatePlayerCamera(float DeltaSeconds);
	void UpdateLights(float DeltaSeconds);
	void UpdateFlipRecovery(float DeltaSeconds, const FVector& Up);
	void ChooseNextAIDirection();
	void SetDormant(bool bNewDormant);
	void StartBurning(AController* DamageInstigator);
	void Explode();

	void Move(const FInputActionValue& Value);
	void StopMove();
	void Look(const FInputActionValue& Value);
	void HandbrakeOn() { bHandbrake = true; }
	void HandbrakeOff() { bHandbrake = false; }
	void Interact();

	UFUNCTION()
	void OnBodyHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ChassisMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CabinMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FrontLightsMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RearLightsMesh;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> Headlight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> FireLight;

	TArray<FVector> WheelOffsets;
	EOCVehicleMode Mode = EOCVehicleMode::Parked;
	TWeakObjectPtr<AOCCharacter> Driver;
	bool bDormant = false;
	bool bPlayerOwned = false;
	bool bHasColor = false;
	FLinearColor BodyColor = FLinearColor::White;

	float ThrottleInput = 0.f;
	float SteerInput = 0.f;
	float SmoothedSteer = 0.f;
	bool bHandbrake = true;
	int32 GroundedWheels = 0;

	float Health = 1000.f;
	bool bBurning = false;
	float BurnTimer = 0.f;
	float LastImpactTime = -10.f;
	float LastLookTime = -10.f;
	float FlipTimer = 0.f;
	float LightCheckTimer = 0.f;
	TWeakObjectPtr<AController> LastDamager;

	FIntPoint AITarget = FIntPoint::ZeroValue;
	FIntPoint AIDir = FIntPoint(1, 0);
	FVector AITurnPoint = FVector::ZeroVector;
	bool bAIHasTurnPoint = false;
	float AICruiseSpeed = 1300.f;
	float AIStuckTimer = 0.f;
};
