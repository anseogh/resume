#include "OCGangMember.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "OCHealthComponent.h"

AOCGangMember::AOCGangMember()
{
	Health->MaxHealth = 110.f;
}

FLinearColor AOCGangMember::GetBlipColor() const
{
	return IsDead() ? FLinearColor::Transparent : FLinearColor(1.f, 0.15f, 0.1f);
}

void AOCGangMember::ReactToThreat(const FVector& ThreatLocation)
{
	if (FVector::Dist(ThreatLocation, GetActorLocation()) < 3500.f)
	{
		bAggro = true;
	}
}

void AOCGangMember::TickAI(float DeltaSeconds)
{
	APawn* Player = GetPlayerPawn();
	if (!Player)
	{
		return;
	}
	const FVector PlayerLoc = Player->GetActorLocation();
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	SightTimer -= DeltaSeconds;
	if (SightTimer <= 0.f)
	{
		SightTimer = 0.35f;
		bCanSeePlayer = DistanceToPlayer < 3500.f && HasLineOfSightTo(Player);
		if (bCanSeePlayer && DistanceToPlayer < 3000.f)
		{
			bAggro = true;
		}
	}

	if (!bAggro)
	{
		return;
	}

	if (!bCanSeePlayer || DistanceToPlayer > 3000.f)
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveToward(PlayerLoc, 500.f, true);
		return;
	}

	MoveComp->bOrientRotationToMovement = false;
	FaceToward(PlayerLoc, DeltaSeconds);

	StrafeTimer -= DeltaSeconds;
	if (StrafeTimer <= 0.f)
	{
		StrafeTimer = FMath::FRandRange(1.f, 2.5f);
		StrafeSign = FMath::RandBool() ? 1.f : -1.f;
	}
	const FVector ToPlayer = (PlayerLoc - GetActorLocation()).GetSafeNormal2D();
	FVector MoveDir = FVector::CrossProduct(FVector::UpVector, ToPlayer) * StrafeSign * 0.6f;
	if (DistanceToPlayer > 1600.f)
	{
		MoveDir += ToPlayer;
	}
	else if (DistanceToPlayer < 700.f)
	{
		MoveDir -= ToPlayer;
	}
	MoveComp->MaxWalkSpeed = 380.f;
	AddMovementInput(MoveDir.GetSafeNormal2D(), 1.f);

	ShootCooldown -= DeltaSeconds;
	if (ShootCooldown <= 0.f)
	{
		ShootAt(Player, 0.3f, 7.f);
		ShootCooldown = FMath::FRandRange(0.6f, 1.3f);
	}
}
