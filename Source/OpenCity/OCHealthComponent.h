#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCHealthComponent.generated.h"

class AController;
class UDamageType;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOCHealthChanged, float /*NewHealth*/, float /*Delta*/, AController* /*Instigator*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOCDeath, AController* /*Killer*/);

UCLASS(ClassGroup = (OpenCity), meta = (BlueprintSpawnableComponent))
class OPENCITY_API UOCHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOCHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	/** Seconds without damage before regeneration starts. 0 disables regeneration. */
	UPROPERTY(EditAnywhere, Category = "Health")
	float RegenDelay = 0.f;

	UPROPERTY(EditAnywhere, Category = "Health")
	float RegenPerSecond = 0.f;

	/** Regeneration only refills up to this fraction of MaxHealth (GTA-style). */
	UPROPERTY(EditAnywhere, Category = "Health")
	float RegenCapFraction = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Health")
	float DamageMultiplier = 1.f;

	FOCHealthChanged OnHealthChanged;
	FOCDeath OnDeath;

	float GetHealth() const { return Health; }
	float GetHealthFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	bool IsDead() const { return bDead; }

	void ApplyDamage(float Amount, AController* DamageInstigator);
	void ResetHealth();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	float Health = 100.f;
	bool bDead = false;
	float TimeSinceDamage = 0.f;
};
