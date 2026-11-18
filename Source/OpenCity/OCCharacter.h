#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "OCCharacter.generated.h"

class AOCVehicle;
class UCameraComponent;
class UOCHealthComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class OPENCITY_API AOCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOCCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UOCHealthComponent* GetHealth() const { return Health; }
	AOCVehicle* GetNearbyVehicle() const { return NearbyVehicle.Get(); }
	AOCVehicle* GetCurrentVehicle() const { return CurrentVehicle.Get(); }
	bool IsAiming() const { return bAiming; }
	bool IsDead() const;

	/** Called by AOCVehicle. Pass nullptr when the character leaves the vehicle. */
	void SetDrivingState(AOCVehicle* Vehicle);

	UPROPERTY(EditAnywhere, Category = "Movement") float RunSpeed = 520.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float SprintSpeed = 820.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float AimWalkSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category = "Combat") float WeaponDamage = 26.f;
	UPROPERTY(EditAnywhere, Category = "Combat") float FireInterval = 0.14f;
	UPROPERTY(EditAnywhere, Category = "Combat") float WeaponRange = 12000.f;

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint() { bSprinting = true; }
	void StopSprint() { bSprinting = false; }
	void StartFire() { bFiring = true; }
	void StopFire() { bFiring = false; }
	void StartAim() { bAiming = true; }
	void StopAim() { bAiming = false; }
	void Interact();

	void FireShot();
	void UpdateNearbyVehicle();
	void HandleDeath(AController* Killer);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VisorMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UOCHealthComponent> Health;

	bool bSprinting = false;
	bool bAiming = false;
	bool bFiring = false;
	float FireCooldown = 0.f;
	float NearbyScanTimer = 0.f;
	TWeakObjectPtr<AOCVehicle> NearbyVehicle;
	TWeakObjectPtr<AOCVehicle> CurrentVehicle;
};
