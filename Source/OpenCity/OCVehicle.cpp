#include "OCVehicle.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "OCAssets.h"
#include "OCCharacter.h"
#include "OCCityGenerator.h"
#include "OCGameMode.h"
#include "OCHUD.h"
#include "OCPedestrian.h"
#include "OCPlayerController.h"
#include "OCPopulationSubsystem.h"
#include "OCText.h"
#include "OCWantedSubsystem.h"
#include "OCWorldEnvironment.h"

namespace
{
	const FLinearColor CarPalette[] = {
		FLinearColor(0.60f, 0.02f, 0.02f), FLinearColor(0.02f, 0.10f, 0.45f), FLinearColor(0.80f, 0.80f, 0.78f),
		FLinearColor(0.03f, 0.03f, 0.03f), FLinearColor(0.35f, 0.36f, 0.38f), FLinearColor(0.85f, 0.55f, 0.05f),
		FLinearColor(0.05f, 0.30f, 0.12f), FLinearColor(0.40f, 0.05f, 0.35f)
	};

	FVector IntDirToVector(const FIntPoint& Dir)
	{
		return FVector(Dir.X, Dir.Y, 0.f);
	}

	FVector RightOf(const FIntPoint& Dir)
	{
		return FVector(-Dir.Y, Dir.X, 0.f);
	}
}

AOCVehicle::AOCVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetBoxExtent(FVector(230.f, 95.f, 45.f));
	Body->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Body->SetNotifyRigidBodyCollision(true);
	Body->SetGenerateOverlapEvents(true);
	Body->SetLinearDamping(0.05f);
	Body->SetAngularDamping(0.8f);

	auto MakeVisual = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(Body);
		Comp->SetStaticMesh(Mesh);
		Comp->SetRelativeLocation(Location);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		return Comp;
	};

	ChassisMesh = MakeVisual(TEXT("Chassis"), OCAssets::Cube(), FVector(0.f, 0.f, -5.f), FVector(4.6f, 1.9f, 0.55f));
	CabinMesh = MakeVisual(TEXT("Cabin"), OCAssets::Cube(), FVector(-25.f, 0.f, 48.f), FVector(2.3f, 1.7f, 0.55f));
	FrontLightsMesh = MakeVisual(TEXT("FrontLights"), OCAssets::Cube(), FVector(231.f, 0.f, 5.f), FVector(0.04f, 1.6f, 0.12f));
	RearLightsMesh = MakeVisual(TEXT("RearLights"), OCAssets::Cube(), FVector(-231.f, 0.f, 5.f), FVector(0.04f, 1.6f, 0.12f));

	WheelOffsets = {
		FVector(150.f, -85.f, -30.f), FVector(150.f, 85.f, -30.f),
		FVector(-150.f, -85.f, -30.f), FVector(-150.f, 85.f, -30.f)
	};
	const TCHAR* WheelNames[] = { TEXT("WheelFL"), TEXT("WheelFR"), TEXT("WheelRL"), TEXT("WheelRR") };
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Wheel = MakeVisual(WheelNames[i], OCAssets::Cylinder(),
			WheelOffsets[i] - FVector(0.f, 0.f, SuspensionRestLength * 0.5f), FVector(0.7f, 0.7f, 0.3f));
		Wheel->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		Wheels.Add(Wheel);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Body);
	CameraBoom->TargetArmLength = 650.f;
	CameraBoom->TargetOffset = FVector(0.f, 0.f, 140.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	Headlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Headlight"));
	Headlight->SetupAttachment(Body);
	Headlight->SetRelativeLocation(FVector(240.f, 0.f, 10.f));
	Headlight->SetRelativeRotation(FRotator(-8.f, 0.f, 0.f));
	Headlight->SetMobility(EComponentMobility::Movable);
	Headlight->SetIntensityUnits(ELightUnits::Candelas);
	Headlight->SetIntensity(3000.f);
	Headlight->SetAttenuationRadius(4500.f);
	Headlight->SetOuterConeAngle(38.f);
	Headlight->SetInnerConeAngle(20.f);
	Headlight->SetCastShadows(false);
	Headlight->SetVisibility(false);

	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(Body);
	FireLight->SetRelativeLocation(FVector(120.f, 0.f, 80.f));
	FireLight->SetMobility(EComponentMobility::Movable);
	FireLight->SetLightColor(FLinearColor(1.f, 0.45f, 0.1f));
	FireLight->SetAttenuationRadius(900.f);
	FireLight->SetCastShadows(false);
	FireLight->SetVisibility(false);
}

void AOCVehicle::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	Body->SetMassOverrideInKg(NAME_None, Mass, true);
	Body->SetCenterOfMass(FVector(0.f, 0.f, -35.f));
	Body->OnComponentHit.AddDynamic(this, &AOCVehicle::OnBodyHit);

	if (!bHasColor)
	{
		BodyColor = CarPalette[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(CarPalette)) - 1)];
		bHasColor = true;
	}
	OCAssets::Paint(ChassisMesh, BodyColor);
	OCAssets::Paint(CabinMesh, FLinearColor(0.02f, 0.025f, 0.03f));
	OCAssets::Paint(FrontLightsMesh, FLinearColor(1.f, 0.95f, 0.8f));
	OCAssets::Paint(RearLightsMesh, FLinearColor(0.6f, 0.f, 0.f));
	for (UStaticMeshComponent* Wheel : Wheels)
	{
		OCAssets::Paint(Wheel, FLinearColor(0.015f, 0.015f, 0.015f));
	}

	SetDormant(Mode == EOCVehicleMode::Parked);
	UpdateDormancy();
}

void AOCVehicle::SetBodyColor(const FLinearColor& Color)
{
	bHasColor = true;
	BodyColor = Color;
	if (HasActorBegunPlay())
	{
		OCAssets::Paint(ChassisMesh, Color);
	}
}

void AOCVehicle::SetDormant(bool bNewDormant)
{
	bDormant = bNewDormant;
	Body->SetSimulatePhysics(!bDormant);
	SetActorTickInterval(bDormant ? 0.5f : 0.f);
	if (bDormant)
	{
		ThrottleInput = SteerInput = SmoothedSteer = 0.f;
	}
	else
	{
		Body->WakeAllRigidBodies();
	}
}

void AOCVehicle::UpdateDormancy()
{
	if (Mode != EOCVehicleMode::Parked)
	{
		return;
	}
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const float Dist = FVector::Dist(Player->GetActorLocation(), GetActorLocation());
	if (bDormant && Dist < WakeDistance)
	{
		SetDormant(false);
	}
	else if (!bDormant && Dist > SleepDistance && GetVelocity().SizeSquared() < 400.f)
	{
		SetDormant(true);
	}
}

void AOCVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateDormancy();
	if (bDormant)
	{
		return;
	}

	if (Mode == EOCVehicleMode::AI)
	{
		UpdateAI(DeltaSeconds);
	}

	if (bBurning)
	{
		BurnTimer -= DeltaSeconds;
		FireLight->SetIntensity(FMath::FRandRange(3000.f, 9000.f));
		if (BurnTimer <= 0.f)
		{
			Explode();
		}
	}

	UpdatePhysics(DeltaSeconds);

	if (Mode == EOCVehicleMode::Player)
	{
		UpdatePlayerCamera(DeltaSeconds);
	}
	UpdateLights(DeltaSeconds);
}

void AOCVehicle::UpdatePhysics(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	const FTransform Transform = Body->GetComponentTransform();
	const FVector Up = Transform.GetUnitAxis(EAxis::Z);
	const FVector Fwd = Transform.GetUnitAxis(EAxis::X);
	const FVector Vel = Body->GetPhysicsLinearVelocity();
	const float Speed = Vel.Size();

	const bool bDrivable = Mode == EOCVehicleMode::Player || Mode == EOCVehicleMode::AI;
	const float Throttle = bDrivable ? ThrottleInput : 0.f;
	SmoothedSteer = FMath::FInterpTo(SmoothedSteer, bDrivable ? SteerInput : 0.f, DeltaSeconds, 7.f);
	const float SteerAngle = SmoothedSteer * MaxSteerAngle * FMath::Lerp(1.f, 0.4f, FMath::Clamp(Speed / 3500.f, 0.f, 1.f));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCWheel), false, this);
	if (Driver.IsValid())
	{
		Params.AddIgnoredActor(Driver.Get());
	}
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	const float WheelMass = Mass * 0.25f;
	const float MaxLength = SuspensionRestLength + WheelRadius;
	GroundedWheels = 0;

	for (int32 i = 0; i < 4; ++i)
	{
		const bool bFront = i < 2;
		const FVector Hardpoint = Transform.TransformPosition(WheelOffsets[i]);
		float SpringLength = SuspensionRestLength;

		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Hardpoint, Hardpoint - Up * MaxLength, ObjParams, Params))
		{
			++GroundedWheels;
			SpringLength = FMath::Clamp(Hit.Distance - WheelRadius, 0.f, SuspensionRestLength);
			const float Compression = 1.f - SpringLength / SuspensionRestLength;
			const FVector PointVel = Body->GetPhysicsLinearVelocityAtPoint(Hardpoint);
			const float SuspensionForce = FMath::Max(0.f, Compression * SpringStrength - FVector::DotProduct(PointVel, Up) * DamperStrength);
			Body->AddForceAtLocation(Up * SuspensionForce, Hardpoint);

			FVector WheelFwd = bFront ? Fwd.RotateAngleAxis(SteerAngle, Up) : Fwd;
			WheelFwd = FVector::VectorPlaneProject(WheelFwd, Hit.ImpactNormal).GetSafeNormal();
			const FVector WheelRight = FVector::CrossProduct(Hit.ImpactNormal, WheelFwd).GetSafeNormal();
			const float LateralVel = FVector::DotProduct(PointVel, WheelRight);
			const float LongVel = FVector::DotProduct(PointVel, WheelFwd);

			const bool bRearHandbrake = bDrivable && bHandbrake && !bFront;
			const float Grip = bFront ? FrontGrip : (bRearHandbrake ? HandbrakeGrip : RearGrip);
			FVector TireForce = -WheelRight * LateralVel * Grip * WheelMass;

			float LongForce = 0.f;
			if (!bDrivable)
			{
				LongForce = -LongVel * 6.f * WheelMass;
			}
			else if (bRearHandbrake)
			{
				LongForce = -LongVel * 2.f * WheelMass;
			}
			else if (FMath::Abs(Throttle) > 0.01f)
			{
				const bool bBraking = (Throttle < 0.f && LongVel > 80.f) || (Throttle > 0.f && LongVel < -80.f);
				if (bBraking)
				{
					LongForce = -FMath::Sign(LongVel) * BrakeForce * 0.25f * FMath::Abs(Throttle);
				}
				else if (Throttle > 0.f || LongVel > -MaxReverseSpeed)
				{
					LongForce = Throttle * EngineForce * 0.25f * (Throttle < 0.f ? 0.5f : 1.f);
				}
			}
			else
			{
				LongForce = -LongVel * RollingResistance * WheelMass;
			}

			TireForce += WheelFwd * LongForce;
			TireForce = TireForce.GetClampedToMaxSize(SuspensionForce * FrictionCoefficient);
			Body->AddForceAtLocation(TireForce, Hardpoint);
		}

		Wheels[i]->SetRelativeLocation(WheelOffsets[i] - FVector(0.f, 0.f, SpringLength));
		Wheels[i]->SetRelativeRotation(FRotator(0.f, bFront ? SteerAngle : 0.f, 90.f));
	}

	Body->AddForce(-Vel * Speed * AirDrag);
	UpdateFlipRecovery(DeltaSeconds, Up);
}

void AOCVehicle::UpdateFlipRecovery(float DeltaSeconds, const FVector& Up)
{
	if (Up.Z > 0.3f || Mode == EOCVehicleMode::Wrecked)
	{
		FlipTimer = 0.f;
		return;
	}

	FlipTimer += DeltaSeconds;
	if (FlipTimer < 2.5f)
	{
		return;
	}
	FlipTimer = 0.f;

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Mode == EOCVehicleMode::AI && Player && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) > 2500.f)
	{
		Destroy();
		return;
	}
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorLocationAndRotation(GetActorLocation() + FVector(0.f, 0.f, 150.f), FRotator(0.f, GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
}

void AOCVehicle::UpdatePlayerCamera(float DeltaSeconds)
{
	AController* Ctrl = GetController();
	if (!Ctrl)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Vel = GetVelocity();
	if (Now - LastLookTime > 1.5f && Vel.SizeSquared2D() > 300.f * 300.f)
	{
		const float FwdSpeed = FVector::DotProduct(Vel, GetActorForwardVector());
		const float TargetYaw = FwdSpeed >= 0.f ? Vel.Rotation().Yaw : GetActorRotation().Yaw;
		const FRotator Current = Ctrl->GetControlRotation();
		const FRotator Target(-12.f, TargetYaw, 0.f);
		Ctrl->SetControlRotation(FMath::RInterpTo(Current, Target, DeltaSeconds, 2.5f));
	}
}

void AOCVehicle::UpdateLights(float DeltaSeconds)
{
	LightCheckTimer -= DeltaSeconds;
	if (LightCheckTimer > 0.f)
	{
		return;
	}
	LightCheckTimer = 1.f;

	bool bNight = false;
	if (AOCGameMode* GM = AOCGameMode::Get(this))
	{
		if (AOCWorldEnvironment* Env = GM->GetEnvironment())
		{
			bNight = Env->IsNight();
		}
	}
	const bool bOccupied = Mode == EOCVehicleMode::Player || Mode == EOCVehicleMode::AI;
	Headlight->SetVisibility(bNight && bOccupied);
}

void AOCVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AOCPlayerController* PC = Cast<AOCPlayerController>(GetController());
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PC || !Input)
	{
		return;
	}
	PC->EnsureInput();

	Input->BindAction(PC->MoveAction, ETriggerEvent::Triggered, this, &AOCVehicle::Move);
	Input->BindAction(PC->MoveAction, ETriggerEvent::Completed, this, &AOCVehicle::StopMove);
	Input->BindAction(PC->LookAction, ETriggerEvent::Triggered, this, &AOCVehicle::Look);
	Input->BindAction(PC->HandbrakeAction, ETriggerEvent::Started, this, &AOCVehicle::HandbrakeOn);
	Input->BindAction(PC->HandbrakeAction, ETriggerEvent::Completed, this, &AOCVehicle::HandbrakeOff);
	Input->BindAction(PC->InteractAction, ETriggerEvent::Started, this, &AOCVehicle::Interact);
}

void AOCVehicle::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	ThrottleInput = FMath::Clamp(Axis.Y, -1.f, 1.f);
	SteerInput = FMath::Clamp(Axis.X, -1.f, 1.f);
}

void AOCVehicle::StopMove()
{
	ThrottleInput = 0.f;
	SteerInput = 0.f;
}

void AOCVehicle::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
	if (!Axis.IsNearlyZero())
	{
		LastLookTime = GetWorld()->GetTimeSeconds();
	}
}

void AOCVehicle::Interact()
{
	if (Mode == EOCVehicleMode::Player)
	{
		ExitVehicle();
	}
}

bool AOCVehicle::EnterVehicle(AOCCharacter* Character)
{
	if (!Character || Mode == EOCVehicleMode::Wrecked || bBurning || Driver.IsValid())
	{
		return false;
	}
	AController* Ctrl = Character->GetController();
	if (!Ctrl)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (Mode == EOCVehicleMode::AI)
	{
		// Carjacking: throw the NPC driver out.
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector EjectLoc = GetActorLocation() + GetActorRightVector() * 260.f + FVector(0.f, 0.f, 30.f);
		if (AOCPedestrian* Ejected = World->SpawnActor<AOCPedestrian>(AOCPedestrian::StaticClass(), EjectLoc, GetActorRotation(), SpawnParams))
		{
			Ejected->ReactToThreat(Character->GetActorLocation());
		}
		if (UOCWantedSubsystem* Wanted = World->GetSubsystem<UOCWantedSubsystem>())
		{
			Wanted->ReportCrime(0.9f, GetActorLocation());
		}
	}
	else if (!bPlayerOwned)
	{
		if (UOCWantedSubsystem* Wanted = World->GetSubsystem<UOCWantedSubsystem>())
		{
			Wanted->ReportCrime(0.35f, GetActorLocation());
		}
	}

	bPlayerOwned = true;
	Driver = Character;
	Character->SetDrivingState(this);
	Mode = EOCVehicleMode::Player;
	SetDormant(false);
	bHandbrake = false;
	ThrottleInput = SteerInput = 0.f;
	LightCheckTimer = 0.f;

	Ctrl->Possess(this);
	Ctrl->SetControlRotation(FRotator(-12.f, GetActorRotation().Yaw, 0.f));
	return true;
}

void AOCVehicle::ExitVehicle()
{
	AOCCharacter* Character = Driver.Get();
	AController* Ctrl = GetController();
	Driver = nullptr;
	Mode = IsWrecked() ? Mode : EOCVehicleMode::Parked;
	ThrottleInput = SteerInput = 0.f;
	bHandbrake = true;
	LightCheckTimer = 0.f;

	if (!Character)
	{
		return;
	}

	const FVector Base = GetActorLocation();
	const FVector Right = GetActorRightVector();
	const FVector Candidates[] = {
		Base - Right * 230.f + FVector(0.f, 0.f, 40.f),
		Base + Right * 230.f + FVector(0.f, 0.f, 40.f),
		Base + FVector(0.f, 0.f, 230.f)
	};

	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCExitVehicle), false, this);
	Params.AddIgnoredActor(Character);
	FVector ExitLocation = Candidates[2];
	for (const FVector& Candidate : Candidates)
	{
		if (!GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(36.f, 90.f), Params))
		{
			ExitLocation = Candidate;
			break;
		}
	}

	Character->SetDrivingState(nullptr);
	Character->SetActorLocationAndRotation(ExitLocation, FRotator(0.f, GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

	if (Ctrl)
	{
		const FRotator ControlRot = Ctrl->GetControlRotation();
		Ctrl->Possess(Character);
		Ctrl->SetControlRotation(ControlRot);
	}
}

void AOCVehicle::StartAIDriving(FIntPoint NextIntersection, FIntPoint Direction)
{
	Mode = EOCVehicleMode::AI;
	AITarget = NextIntersection;
	AIDir = Direction;
	bAIHasTurnPoint = false;
	AICruiseSpeed = FMath::FRandRange(1050.f, 1500.f);
	bHandbrake = false;
	if (bDormant || !Body->IsSimulatingPhysics())
	{
		SetDormant(false);
	}
}

void AOCVehicle::ChooseNextAIDirection()
{
	AOCGameMode* GM = AOCGameMode::Get(this);
	AOCCityGenerator* City = GM ? GM->GetCity() : nullptr;
	if (!City)
	{
		return;
	}

	const FIntPoint Options[] = { AIDir, FIntPoint(-AIDir.Y, AIDir.X), FIntPoint(AIDir.Y, -AIDir.X) };
	const float Weights[] = { 0.55f, 0.225f, 0.225f };
	TArray<int32> Valid;
	float TotalWeight = 0.f;
	for (int32 i = 0; i < 3; ++i)
	{
		if (City->IsValidIntersection(AITarget + Options[i]))
		{
			Valid.Add(i);
			TotalWeight += Weights[i];
		}
	}

	FIntPoint NewDir = FIntPoint(-AIDir.X, -AIDir.Y);
	if (Valid.Num() > 0)
	{
		float Roll = FMath::FRand() * TotalWeight;
		NewDir = Options[Valid.Last()];
		for (int32 Index : Valid)
		{
			Roll -= Weights[Index];
			if (Roll <= 0.f)
			{
				NewDir = Options[Index];
				break;
			}
		}
	}

	const float Lane = City->GetLaneOffset();
	const FVector Junction = City->GetIntersectionLocation(AITarget);
	bAIHasTurnPoint = NewDir != AIDir;
	AITurnPoint = Junction + RightOf(AIDir) * Lane + RightOf(NewDir) * Lane;
	AIDir = NewDir;
	AITarget = AITarget + NewDir;
}

void AOCVehicle::UpdateAI(float DeltaSeconds)
{
	AOCGameMode* GM = AOCGameMode::Get(this);
	AOCCityGenerator* City = GM ? GM->GetCity() : nullptr;
	if (!City)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	const float Lane = City->GetLaneOffset();
	const FVector Junction = City->GetIntersectionLocation(AITarget);
	const FVector LanePoint = Junction + RightOf(AIDir) * Lane;

	if (bAIHasTurnPoint && FVector::Dist2D(Location, AITurnPoint) < 350.f)
	{
		bAIHasTurnPoint = false;
	}
	if (!bAIHasTurnPoint && FVector::DotProduct(LanePoint - Location, IntDirToVector(AIDir)) < 900.f)
	{
		ChooseNextAIDirection();
	}

	const FVector Target = bAIHasTurnPoint ? AITurnPoint : City->GetIntersectionLocation(AITarget) + RightOf(AIDir) * Lane;
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Target);
	const float Angle = FMath::Atan2(Local.Y, Local.X);
	SteerInput = FMath::Clamp(Angle / FMath::DegreesToRadians(30.f), -1.f, 1.f);

	float DesiredSpeed = AICruiseSpeed * FMath::Lerp(1.f, 0.35f, FMath::Clamp(FMath::Abs(Angle) / 1.2f, 0.f, 1.f));
	if (bAIHasTurnPoint)
	{
		DesiredSpeed = FMath::Min(DesiredSpeed, 650.f);
	}

	const FVector Fwd = GetActorForwardVector();
	const float FwdSpeed = FVector::DotProduct(GetVelocity(), Fwd);
	FCollisionObjectQueryParams Obstacles;
	Obstacles.AddObjectTypesToQuery(ECC_Pawn);
	Obstacles.AddObjectTypesToQuery(ECC_Vehicle);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCTrafficSensor), false, this);
	const FVector SensorStart = Location + Fwd * 245.f;
	const float SensorLength = 350.f + FMath::Max(0.f, FwdSpeed) * 0.9f;
	FHitResult Hit;
	if (GetWorld()->SweepSingleByObjectType(Hit, SensorStart, SensorStart + Fwd * SensorLength, FQuat::Identity, Obstacles, FCollisionShape::MakeSphere(70.f), Params))
	{
		DesiredSpeed = Hit.Distance < 250.f ? 0.f : FMath::Min(DesiredSpeed, Hit.Distance * 0.8f);
	}

	ThrottleInput = FMath::Clamp((DesiredSpeed - FwdSpeed) / 400.f, -1.f, 1.f);

	if (DesiredSpeed > 300.f && FwdSpeed < 60.f)
	{
		AIStuckTimer += DeltaSeconds;
		if (AIStuckTimer > 8.f)
		{
			const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
			if (!Player || FVector::Dist(Player->GetActorLocation(), Location) > 3000.f)
			{
				Destroy();
			}
			AIStuckTimer = 0.f;
		}
	}
	else
	{
		AIStuckTimer = 0.f;
	}
}

float AOCVehicle::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Mode == EOCVehicleMode::Wrecked || Actual <= 0.f)
	{
		return Actual;
	}

	if (bDormant)
	{
		SetDormant(false);
	}
	Health -= Actual;
	if (EventInstigator)
	{
		LastDamager = EventInstigator;
	}
	if (Health <= 0.f)
	{
		StartBurning(EventInstigator);
	}
	return Actual;
}

void AOCVehicle::OnBodyHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (AOCVehicle* OtherCar = Cast<AOCVehicle>(OtherActor))
	{
		if (OtherCar->bDormant)
		{
			OtherCar->SetDormant(false);
		}
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastImpactTime < 0.3f)
	{
		return;
	}

	const float DeltaV = NormalImpulse.Size() / FMath::Max(Mass, 1.f);
	if (DeltaV > 700.f)
	{
		LastImpactTime = Now;
		FDamageEvent DamageEvent;
		TakeDamage((DeltaV - 700.f) * 0.15f, DamageEvent, GetController(), OtherActor);
	}
}

void AOCVehicle::StartBurning(AController* DamageInstigator)
{
	if (bBurning || Mode == EOCVehicleMode::Wrecked)
	{
		return;
	}
	bBurning = true;
	BurnTimer = 4.5f;
	LastDamager = DamageInstigator;
	FireLight->SetVisibility(true);
	if (Mode == EOCVehicleMode::Player)
	{
		AOCHUD::ShowNotification(this, OCText::VehicleOnFire, 4.f);
	}
}

void AOCVehicle::Explode()
{
	bBurning = false;
	AController* DamageInstigator = LastDamager.Get();

	if (Driver.IsValid())
	{
		ExitVehicle();
	}
	Mode = EOCVehicleMode::Wrecked;
	ThrottleInput = SteerInput = 0.f;
	Headlight->SetVisibility(false);
	FireLight->SetIntensity(2500.f);
	OCAssets::Paint(ChassisMesh, FLinearColor(0.02f, 0.02f, 0.02f));
	OCAssets::Paint(CabinMesh, FLinearColor(0.01f, 0.01f, 0.01f));

	Body->AddImpulse(FVector(0.f, 0.f, 650.f), NAME_None, true);
	Body->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), 0.f), NAME_None, true);

	const TArray<AActor*> Ignore = { this };
	UGameplayStatics::ApplyRadialDamage(this, 250.f, GetActorLocation(), 900.f, UDamageType::StaticClass(), Ignore, this, DamageInstigator, false, ECC_Visibility);

	UWorld* World = GetWorld();
	if (UOCPopulationSubsystem* Population = World->GetSubsystem<UOCPopulationSubsystem>())
	{
		Population->BroadcastThreat(GetActorLocation(), 4000.f);
	}
	if (DamageInstigator && DamageInstigator->IsPlayerController())
	{
		if (UOCWantedSubsystem* Wanted = World->GetSubsystem<UOCWantedSubsystem>())
		{
			Wanted->ReportCrime(1.f, GetActorLocation());
		}
	}
	SetLifeSpan(45.f);
}
