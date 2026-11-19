#include "OCMarker.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "OCAssets.h"

AOCMarker::AOCMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(RootComponent);
	Ring->SetStaticMesh(OCAssets::Cylinder());
	Ring->SetRelativeScale3D(FVector(3.f, 3.f, 0.05f));
	Ring->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	Ring->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Ring->SetCastShadow(false);

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(RootComponent);
	Beam->SetStaticMesh(OCAssets::Cylinder());
	Beam->SetRelativeScale3D(FVector(0.35f, 0.35f, 60.f));
	Beam->SetRelativeLocation(FVector(0.f, 0.f, 3000.f));
	Beam->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Beam->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	Light->SetAttenuationRadius(900.f);
	Light->SetIntensity(6000.f);
	Light->SetCastShadows(false);
}

void AOCMarker::SetColor(const FLinearColor& Color)
{
	OCAssets::Paint(Ring, Color);
	OCAssets::Paint(Beam, Color);
	Light->SetLightColor(Color);
}

void AOCMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	const float Pulse = 1.f + 0.12f * FMath::Sin(Time * 3.f);
	Ring->SetRelativeScale3D(FVector(3.f * Pulse, 3.f * Pulse, 0.05f));
}
