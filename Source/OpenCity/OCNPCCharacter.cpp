#include "OCNPCCharacter.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "OCAssets.h"
#include "OCCityGenerator.h"
#include "OCGameMode.h"
#include "OCHealthComponent.h"
#include "OCPopulationSubsystem.h"
#include "OCVehicle.h"
#include "OCWantedSubsystem.h"

AOCNPCCharacter::AOCNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(34.f, 90.f);
	// Cars pass through NPC capsules and run them over instead of stopping dead on a kinematic wall.
	Capsule->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	MoveComp->MaxWalkSpeed = 150.f;
	MoveComp->bUseRVOAvoidance = true;
	MoveComp->AvoidanceConsiderationRadius = 300.f;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(Capsule);
	BodyMesh->SetStaticMesh(OCAssets::Cylinder());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -25.f));
	BodyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.3f));
	BodyMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(BodyMesh);
	HeadMesh->SetStaticMesh(OCAssets::Sphere());
	HeadMesh->SetUsingAbsoluteScale(true);
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	HeadMesh->SetRelativeScale3D(FVector(0.36f));
	HeadMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	HatMesh->SetupAttachment(HeadMesh);
	HatMesh->SetStaticMesh(OCAssets::Cylinder());
	HatMesh->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	HatMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, 0.35f));
	HatMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	HatMesh->SetVisibility(false);

	Health = CreateDefaultSubobject<UOCHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 100.f;
}

void AOCNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	OCAssets::Paint(BodyMesh, GetClothingColor());
	const float Skin = FMath::FRandRange(0.25f, 0.75f);
	OCAssets::Paint(HeadMesh, FLinearColor(Skin, Skin * 0.72f, Skin * 0.55f));

	Health->OnHealthChanged.AddUObject(this, &AOCNPCCharacter::OnDamaged);
	Health->OnDeath.AddUObject(this, &AOCNPCCharacter::OnKilled);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AOCNPCCharacter::OnCapsuleOverlap);

	if (UOCPopulationSubsystem* Population = GetWorld()->GetSubsystem<UOCPopulationSubsystem>())
	{
		Population->RegisterNPC(this);
	}
}

void AOCNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UOCPopulationSubsystem* Population = World->GetSubsystem<UOCPopulationSubsystem>())
		{
			Population->UnregisterNPC(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool AOCNPCCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

void AOCNPCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsDead())
	{
		return;
	}
	UpdateSignificance();
	TickAI(DeltaSeconds);
}

void AOCNPCCharacter::UpdateSignificance()
{
	const APawn* Player = GetPlayerPawn();
	DistanceToPlayer = Player ? FVector::Dist(Player->GetActorLocation(), GetActorLocation()) : 0.f;

	const int32 Level = DistanceToPlayer < 3000.f ? 0 : (DistanceToPlayer < 7000.f ? 1 : 2);
	if (Level == SignificanceLevel)
	{
		return;
	}
	SignificanceLevel = Level;

	const float ActorInterval = Level == 0 ? 0.f : (Level == 1 ? 0.1f : 0.3f);
	SetActorTickInterval(ActorInterval);
	GetCharacterMovement()->SetComponentTickInterval(Level == 2 ? 0.1f : 0.f);
	const bool bShadows = Level < 2;
	BodyMesh->SetCastShadow(bShadows);
	HeadMesh->SetCastShadow(bShadows);
	HatMesh->SetCastShadow(bShadows);
}

APawn* AOCNPCCharacter::GetPlayerPawn() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

AOCCityGenerator* AOCNPCCharacter::GetCity() const
{
	if (!CachedCity.IsValid())
	{
		if (AOCGameMode* GM = AOCGameMode::Get(this))
		{
			CachedCity = GM->GetCity();
		}
	}
	return CachedCity.Get();
}

void AOCNPCCharacter::MoveToward(const FVector& Target, float Speed, bool bUseNavigation)
{
	FVector Goal = Target;
	if (bUseNavigation)
	{
		if (const AOCCityGenerator* City = GetCity())
		{
			Goal = City->GetNavWaypoint(GetActorLocation(), Target);
		}
	}
	const FVector Dir = (Goal - GetActorLocation()).GetSafeNormal2D();
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	AddMovementInput(Dir, 1.f);
}

void AOCNPCCharacter::FaceToward(const FVector& Target, float DeltaSeconds)
{
	const FVector Dir = (Target - GetActorLocation()).GetSafeNormal2D();
	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Dir.Rotation(), DeltaSeconds, 10.f));
	}
}

bool AOCNPCCharacter::HasLineOfSightTo(const AActor* Target) const
{
	if (!Target)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCNPCSight), false, this);
	Params.AddIgnoredActor(Target);
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector TargetLoc = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	return !GetWorld()->LineTraceTestByObjectType(Eye, TargetLoc, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

void AOCNPCCharacter::ShootAt(AActor* Target, float HitChance, float Damage)
{
	if (!Target)
	{
		return;
	}
	UWorld* World = GetWorld();
	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 45.f + FVector(0.f, 0.f, 40.f);
	FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 30.f);
	const bool bHit = FMath::FRand() < HitChance;
	if (!bHit)
	{
		Aim += FMath::VRand() * FMath::FRandRange(90.f, 260.f);
	}

	DrawDebugLine(World, Muzzle, Aim, FColor(255, 200, 90), false, 0.05f, 0, 1.f);
	if (bHit)
	{
		const FVector Dir = (Aim - Muzzle).GetSafeNormal();
		UGameplayStatics::ApplyPointDamage(Target, Damage, Dir, FHitResult(), GetController(), this, UDamageType::StaticClass());
	}

	if (UOCPopulationSubsystem* Population = World->GetSubsystem<UOCPopulationSubsystem>())
	{
		Population->BroadcastThreat(GetActorLocation(), 2500.f);
	}
}

void AOCNPCCharacter::OnDamaged(float NewHealth, float Delta, AController* DamageInstigator)
{
	APawn* InstigatorPawn = DamageInstigator ? DamageInstigator->GetPawn() : nullptr;
	if (InstigatorPawn)
	{
		DeathImpulse = (GetActorLocation() - InstigatorPawn->GetActorLocation()).GetSafeNormal2D() * 300.f + FVector(0.f, 0.f, 150.f);
		ReactToThreat(InstigatorPawn->GetActorLocation());
	}

	if (DamageInstigator && DamageInstigator->IsPlayerController())
	{
		const float Heat = NewHealth > 0.f ? GetCrimeHeatForDamage() : 0.f;
		if (Heat > 0.f)
		{
			if (UOCWantedSubsystem* Wanted = GetWorld()->GetSubsystem<UOCWantedSubsystem>())
			{
				Wanted->ReportCrime(Heat, GetActorLocation());
			}
		}
	}
}

void AOCNPCCharacter::OnKilled(AController* Killer)
{
	UWorld* World = GetWorld();
	if (Killer && Killer->IsPlayerController())
	{
		if (UOCWantedSubsystem* Wanted = World->GetSubsystem<UOCWantedSubsystem>())
		{
			Wanted->ReportCrime(GetCrimeHeatForKill(), GetActorLocation());
		}
	}
	if (UOCPopulationSubsystem* Population = World->GetSubsystem<UOCPopulationSubsystem>())
	{
		Population->BroadcastThreat(GetActorLocation(), 2500.f);
	}

	Ragdoll();
	DetachFromControllerPendingDestroy();
	SetLifeSpan(20.f);
}

void AOCNPCCharacter::Ragdoll()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
	MoveComp->SetComponentTickEnabled(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BodyMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	BodyMesh->SetSimulatePhysics(true);
	BodyMesh->AddImpulse(DeathImpulse, NAME_None, true);
	BodyMesh->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), 0.f), NAME_None, true);
}

void AOCNPCCharacter::OnCapsuleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AOCVehicle* Car = Cast<AOCVehicle>(OtherActor);
	if (!Car || IsDead())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector CarVel = Car->GetVelocity();
	const float Speed = CarVel.Size();
	if (Speed < 350.f || Now - LastRunOverTime < 0.5f)
	{
		ReactToThreat(Car->GetActorLocation());
		return;
	}
	LastRunOverTime = Now;

	const FVector Launch = CarVel * 0.8f + FVector(0.f, 0.f, 350.f);
	DeathImpulse = Launch;
	UGameplayStatics::ApplyDamage(this, Speed * 0.07f, Car->GetController(), Car, UDamageType::StaticClass());
	if (!IsDead())
	{
		LaunchCharacter(Launch, true, true);
	}
}
