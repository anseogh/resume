#include "OCHealthComponent.h"
#include "GameFramework/Actor.h"

UOCHealthComponent::UOCHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
}

void UOCHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UOCHealthComponent::HandleAnyDamage);
	}
}

void UOCHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	ApplyDamage(Damage, InstigatedBy);
}

void UOCHealthComponent::ApplyDamage(float Amount, AController* DamageInstigator)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}

	const float Actual = FMath::Min(Health, Amount * DamageMultiplier);
	Health -= Actual;
	TimeSinceDamage = 0.f;
	OnHealthChanged.Broadcast(Health, -Actual, DamageInstigator);

	if (Health <= 0.f)
	{
		bDead = true;
		OnDeath.Broadcast(DamageInstigator);
	}
}

void UOCHealthComponent::ResetHealth()
{
	bDead = false;
	Health = MaxHealth;
	TimeSinceDamage = 0.f;
	OnHealthChanged.Broadcast(Health, 0.f, nullptr);
}

void UOCHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDead || RegenDelay <= 0.f || RegenPerSecond <= 0.f)
	{
		return;
	}

	TimeSinceDamage += DeltaTime;
	const float Cap = MaxHealth * RegenCapFraction;
	if (TimeSinceDamage >= RegenDelay && Health < Cap)
	{
		Health = FMath::Min(Cap, Health + RegenPerSecond * DeltaTime);
	}
}
