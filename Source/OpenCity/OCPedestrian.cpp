#include "OCPedestrian.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "OCCityGenerator.h"

AOCPedestrian::AOCPedestrian()
{
}

void AOCPedestrian::BeginPlay()
{
	Clothing = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::RandRange(0, 255)), static_cast<uint8>(FMath::RandRange(60, 200)), static_cast<uint8>(FMath::RandRange(60, 220)));
	WalkSpeed = FMath::FRandRange(120.f, 175.f);
	bClockwise = FMath::RandBool();
	Super::BeginPlay();
}

void AOCPedestrian::InitRoute(FIntPoint InBlock, int32 InCorner)
{
	Block = InBlock;
	Corner = InCorner;
}

void AOCPedestrian::EnsureRoute()
{
	const AOCCityGenerator* City = GetCity();
	if (!City || City->IsValidBlock(Block))
	{
		return;
	}
	City->FindNearestSidewalkCorner(GetActorLocation(), Block, Corner);
}

void AOCPedestrian::ReactToThreat(const FVector& ThreatLocation)
{
	if (IsDead() || FVector::Dist(ThreatLocation, GetActorLocation()) > 4000.f)
	{
		return;
	}
	PanicTime = FMath::FRandRange(5.f, 9.f);
	ThreatOrigin = ThreatLocation;
	PauseTime = 0.f;
}

void AOCPedestrian::TickAI(float DeltaSeconds)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (PanicTime > 0.f)
	{
		PanicTime -= DeltaSeconds;
		FVector Away = (GetActorLocation() - ThreatOrigin).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = GetActorForwardVector();
		}
		MoveComp->MaxWalkSpeed = 560.f;
		AddMovementInput(Away, 1.f);
		if (PanicTime <= 0.f)
		{
			Block = FIntPoint(-1, -1);
		}
		return;
	}

	if (PauseTime > 0.f)
	{
		PauseTime -= DeltaSeconds;
		return;
	}

	EnsureRoute();
	const AOCCityGenerator* City = GetCity();
	if (!City || !City->IsValidBlock(Block))
	{
		return;
	}

	const FVector Target = City->GetSidewalkCorner(Block, Corner);
	if (FVector::Dist2D(Target, GetActorLocation()) < 120.f)
	{
		AdvanceRoute();
		if (FMath::FRand() < 0.15f)
		{
			PauseTime = FMath::FRandRange(1.f, 4.f);
		}
		return;
	}
	MoveToward(Target, WalkSpeed, false);
}

void AOCPedestrian::AdvanceRoute()
{
	const AOCCityGenerator* City = GetCity();
	if (!City)
	{
		return;
	}

	// Corners: 0 = (minX,minY), 1 = (maxX,minY), 2 = (maxX,maxY), 3 = (minX,maxY).
	if (FMath::FRand() < 0.2f)
	{
		const bool bAcrossX = FMath::RandBool();
		FIntPoint NewBlock = Block;
		int32 NewCorner = Corner;
		if (bAcrossX)
		{
			const bool bMaxX = Corner == 1 || Corner == 2;
			NewBlock.X += bMaxX ? 1 : -1;
			NewCorner = (Corner == 0) ? 1 : (Corner == 1) ? 0 : (Corner == 2) ? 3 : 2;
		}
		else
		{
			const bool bMaxY = Corner == 2 || Corner == 3;
			NewBlock.Y += bMaxY ? 1 : -1;
			NewCorner = (Corner == 0) ? 3 : (Corner == 3) ? 0 : (Corner == 1) ? 2 : 1;
		}
		if (City->IsValidBlock(NewBlock))
		{
			Block = NewBlock;
			Corner = NewCorner;
			return;
		}
	}

	Corner = (Corner + (bClockwise ? 1 : 3)) % 4;
}
