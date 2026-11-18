#include "OCPoliceOfficer.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "OCAssets.h"
#include "OCHealthComponent.h"
#include "OCWantedSubsystem.h"

AOCPoliceOfficer::AOCPoliceOfficer()
{
	Health->MaxHealth = 120.f;
	HatMesh->SetVisibility(true);
}

void AOCPoliceOfficer::BeginPlay()
{
	Super::BeginPlay();
	OCAssets::Paint(HatMesh, FLinearColor(0.01f, 0.01f, 0.03f));
}

FLinearColor AOCPoliceOfficer::GetBlipColor() const
{
	if (IsDead())
	{
		return FLinearColor::Transparent;
	}
	const bool bRed = FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.5f) < 0.25f;
	return bRed ? FLinearColor(1.f, 0.1f, 0.1f) : FLinearColor(0.15f, 0.35f, 1.f);
}

void AOCPoliceOfficer::TickAI(float DeltaSeconds)
{
	UOCWantedSubsystem* Wanted = GetWorld()->GetSubsystem<UOCWantedSubsystem>();
	APawn* Player = GetPlayerPawn();
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (!Wanted || !Player || Wanted->GetStars() == 0)
	{
		bCanSeePlayer = false;
		MoveComp->bOrientRotationToMovement = true;
		return;
	}

	const FVector PlayerLoc = Player->GetActorLocation();
	SightTimer -= DeltaSeconds;
	if (SightTimer <= 0.f)
	{
		SightTimer = 0.3f;
		bCanSeePlayer = DistanceToPlayer < 6000.f && HasLineOfSightTo(Player);
		if (bCanSeePlayer)
		{
			Wanted->NotifyPlayerSpotted(PlayerLoc);
		}
	}

	if (bCanSeePlayer && DistanceToPlayer < 2600.f)
	{
		MoveComp->bOrientRotationToMovement = false;
		FaceToward(PlayerLoc, DeltaSeconds);
		if (DistanceToPlayer > 1000.f)
		{
			MoveToward(PlayerLoc, 520.f, false);
		}

		ShootCooldown -= DeltaSeconds;
		if (ShootCooldown <= 0.f)
		{
			const float TargetSpeed = Player->GetVelocity().Size();
			const float HitChance = FMath::Clamp(0.45f - TargetSpeed / 4000.f, 0.08f, 0.45f);
			ShootAt(Player, HitChance, 5.f + 2.f * Wanted->GetStars());
			ShootCooldown = FMath::FRandRange(0.7f, 1.4f);
		}
		return;
	}

	MoveComp->bOrientRotationToMovement = true;
	const FVector Goal = bCanSeePlayer ? PlayerLoc : Wanted->GetLastKnownLocation();
	if (FVector::Dist2D(Goal, GetActorLocation()) > 300.f)
	{
		MoveToward(Goal, 600.f, true);
	}
}
