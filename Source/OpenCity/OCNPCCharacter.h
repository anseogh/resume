#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OCNPCCharacter.generated.h"

class AOCCityGenerator;
class UOCHealthComponent;
class UStaticMeshComponent;

/**
 * Base for every AI human. Handles visuals, health, crime reporting, ragdoll death,
 * being run over, and distance-based tick LOD ("significance").
 */
UCLASS(Abstract)
class OPENCITY_API AOCNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOCNPCCharacter();

	virtual void Tick(float DeltaSeconds) override;

	UOCHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const;

	virtual void ReactToThreat(const FVector& ThreatLocation) {}

	/** Minimap blip colour; transparent means "no blip". */
	virtual FLinearColor GetBlipColor() const { return FLinearColor::Transparent; }

	/** Witnesses can report crimes to the police. */
	virtual bool IsWitness() const { return false; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickAI(float DeltaSeconds) {}
	virtual void OnDamaged(float NewHealth, float Delta, AController* DamageInstigator);
	virtual void OnKilled(AController* Killer);
	virtual float GetCrimeHeatForDamage() const { return 0.f; }
	virtual float GetCrimeHeatForKill() const { return 0.f; }
	virtual FLinearColor GetClothingColor() const { return FLinearColor(0.3f, 0.3f, 0.3f); }

	void MoveToward(const FVector& Target, float Speed, bool bUseNavigation);
	void FaceToward(const FVector& Target, float DeltaSeconds);
	bool HasLineOfSightTo(const AActor* Target) const;
	void ShootAt(AActor* Target, float HitChance, float Damage);
	APawn* GetPlayerPawn() const;
	AOCCityGenerator* GetCity() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HatMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UOCHealthComponent> Health;

	float DistanceToPlayer = 0.f;

private:
	UFUNCTION()
	void OnCapsuleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void UpdateSignificance();
	void Ragdoll();

	int32 SignificanceLevel = -1;
	FVector DeathImpulse = FVector::ZeroVector;
	float LastRunOverTime = -10.f;
	mutable TWeakObjectPtr<AOCCityGenerator> CachedCity;
};
