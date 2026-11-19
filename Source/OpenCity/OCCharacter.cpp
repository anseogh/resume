#include "OCCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "OCAssets.h"
#include "OCGameMode.h"
#include "OCHealthComponent.h"
#include "OCPlayerController.h"
#include "OCPopulationSubsystem.h"
#include "OCVehicle.h"
#include "OCWantedSubsystem.h"

AOCCharacter::AOCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);
	MoveComp->MaxWalkSpeed = RunSpeed;
	MoveComp->JumpZVelocity = 520.f;
	MoveComp->AirControl = 0.3f;
	MoveComp->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.f;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetStaticMesh(OCAssets::Cylinder());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -25.f));
	BodyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.3f));
	BodyMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetStaticMesh(OCAssets::Sphere());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 58.f));
	HeadMesh->SetRelativeScale3D(FVector(0.36f));
	HeadMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	VisorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visor"));
	VisorMesh->SetupAttachment(GetCapsuleComponent());
	VisorMesh->SetStaticMesh(OCAssets::Cube());
	VisorMesh->SetRelativeLocation(FVector(15.f, 0.f, 62.f));
	VisorMesh->SetRelativeScale3D(FVector(0.08f, 0.26f, 0.08f));
	VisorMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	Health = CreateDefaultSubobject<UOCHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 200.f;
	Health->RegenDelay = 6.f;
	Health->RegenPerSecond = 6.f;
	Health->RegenCapFraction = 0.5f;
}

void AOCCharacter::BeginPlay()
{
	Super::BeginPlay();

	OCAssets::Paint(BodyMesh, FLinearColor(0.05f, 0.06f, 0.08f));
	OCAssets::Paint(HeadMesh, FLinearColor(0.62f, 0.45f, 0.33f));
	OCAssets::Paint(VisorMesh, FLinearColor(0.02f, 0.02f, 0.02f));

	Health->OnDeath.AddUObject(this, &AOCCharacter::HandleDeath);
}

bool AOCCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

void AOCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AOCPlayerController* PC = Cast<AOCPlayerController>(GetController());
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PC || !Input)
	{
		return;
	}
	PC->EnsureInput();

	Input->BindAction(PC->MoveAction, ETriggerEvent::Triggered, this, &AOCCharacter::Move);
	Input->BindAction(PC->LookAction, ETriggerEvent::Triggered, this, &AOCCharacter::Look);
	Input->BindAction(PC->JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(PC->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(PC->SprintAction, ETriggerEvent::Started, this, &AOCCharacter::StartSprint);
	Input->BindAction(PC->SprintAction, ETriggerEvent::Completed, this, &AOCCharacter::StopSprint);
	Input->BindAction(PC->FireAction, ETriggerEvent::Started, this, &AOCCharacter::StartFire);
	Input->BindAction(PC->FireAction, ETriggerEvent::Completed, this, &AOCCharacter::StopFire);
	Input->BindAction(PC->AimAction, ETriggerEvent::Started, this, &AOCCharacter::StartAim);
	Input->BindAction(PC->AimAction, ETriggerEvent::Completed, this, &AOCCharacter::StopAim);
	Input->BindAction(PC->InteractAction, ETriggerEvent::Started, this, &AOCCharacter::Interact);
}

void AOCCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || IsDead())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AOCCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AOCCharacter::Interact()
{
	if (IsDead() || CurrentVehicle.IsValid())
	{
		return;
	}
	UpdateNearbyVehicle();
	if (AOCVehicle* Vehicle = NearbyVehicle.Get())
	{
		Vehicle->EnterVehicle(this);
	}
}

void AOCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsDead() || CurrentVehicle.IsValid())
	{
		return;
	}

	FireCooldown -= DeltaSeconds;
	if (bFiring && FireCooldown <= 0.f)
	{
		FireShot();
		FireCooldown = FireInterval;
	}

	const bool bCombatStance = bAiming || bFiring;
	bUseControllerRotationYaw = bCombatStance;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = !bCombatStance;
	MoveComp->MaxWalkSpeed = bAiming ? AimWalkSpeed : (bSprinting ? SprintSpeed : RunSpeed);

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, bAiming ? 190.f : 380.f, DeltaSeconds, 10.f);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, bAiming ? FVector(0.f, 70.f, 60.f) : FVector(0.f, 50.f, 70.f), DeltaSeconds, 10.f);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, bAiming ? 65.f : 90.f, DeltaSeconds, 10.f));

	NearbyScanTimer -= DeltaSeconds;
	if (NearbyScanTimer <= 0.f)
	{
		NearbyScanTimer = 0.2f;
		UpdateNearbyVehicle();
	}
}

void AOCCharacter::UpdateNearbyVehicle()
{
	NearbyVehicle = nullptr;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCVehicleScan), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Vehicle), FCollisionShape::MakeSphere(420.f), Params);

	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AOCVehicle* Vehicle = Cast<AOCVehicle>(Overlap.GetActor());
		if (!Vehicle || Vehicle->IsWrecked() || Vehicle->IsBurning())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Vehicle->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			NearbyVehicle = Vehicle;
		}
	}
}

void AOCCharacter::FireShot()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	SetActorRotation(FRotator(0.f, CamRot.Yaw, 0.f));

	const float SpreadDeg = bAiming ? 0.35f : 1.8f;
	const FVector Dir = FMath::VRandCone(CamRot.Vector(), FMath::DegreesToRadians(SpreadDeg));
	const FVector End = CamLoc + Dir * WeaponRange;

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjParams.AddObjectTypesToQuery(ECC_Vehicle);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCPlayerShot), true, this);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByObjectType(Hit, CamLoc, End, ObjParams, Params);
	const FVector Impact = bHit ? Hit.ImpactPoint : End;
	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 40.f + GetActorRightVector() * 20.f + FVector(0.f, 0.f, 45.f);

	DrawDebugLine(World, Muzzle, Impact, FColor(255, 220, 120), false, 0.04f, 0, 1.2f);
	if (bHit)
	{
		DrawDebugPoint(World, Impact, 8.f, FColor::Orange, false, 0.08f);
		if (AActor* HitActor = Hit.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(HitActor, WeaponDamage, Dir, Hit, PC, this, UDamageType::StaticClass());
		}
		if (UPrimitiveComponent* HitComp = Hit.GetComponent())
		{
			if (HitComp->IsSimulatingPhysics())
			{
				HitComp->AddImpulseAtLocation(Dir * 25000.f, Impact);
			}
		}
	}

	if (UOCPopulationSubsystem* Population = World->GetSubsystem<UOCPopulationSubsystem>())
	{
		Population->BroadcastThreat(GetActorLocation(), 3500.f);
	}
	if (UOCWantedSubsystem* Wanted = World->GetSubsystem<UOCWantedSubsystem>())
	{
		Wanted->ReportCrime(0.15f, GetActorLocation());
	}
}

void AOCCharacter::SetDrivingState(AOCVehicle* Vehicle)
{
	CurrentVehicle = Vehicle;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (Vehicle)
	{
		bFiring = bAiming = bSprinting = false;
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		SetActorEnableCollision(false);
		SetActorHiddenInGame(true);
		AttachToActor(Vehicle, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}

void AOCCharacter::HandleDeath(AController* Killer)
{
	bFiring = false;
	GetCharacterMovement()->DisableMovement();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	BodyMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	BodyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	BodyMesh->SetSimulatePhysics(true);
	BodyMesh->AddImpulse(GetActorForwardVector() * -150.f + FVector(0.f, 0.f, 100.f), NAME_None, true);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
		if (AOCGameMode* GM = AOCGameMode::Get(this))
		{
			GM->HandlePlayerDeath(PC);
		}
	}
}
