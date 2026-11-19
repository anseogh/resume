#include "OCWorldEnvironment.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"

AOCWorldEnvironment::AOCWorldEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->bAtmosphereSunLight = true;
	Sun->AtmosphereSunLightIndex = 0;
	Sun->Intensity = 10.f;
	Sun->LightSourceAngle = 0.5f;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(RootComponent);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->bAtmosphereSunLight = true;
	Moon->AtmosphereSunLightIndex = 1;
	Moon->Intensity = 0.f;
	Moon->LightColor = FColor(150, 170, 255);
	Moon->SetCastShadows(false);

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	Fog->SetupAttachment(RootComponent);
	Fog->FogDensity = 0.012f;
	Fog->FogHeightFalloff = 0.15f;
	Fog->bEnableVolumetricFog = true;
}

void AOCWorldEnvironment::BeginPlay()
{
	Super::BeginPlay();
	UpdateLighting();
}

void AOCWorldEnvironment::SetTimeOfDay(float Hour)
{
	TimeOfDay = FMath::Fmod(FMath::Max(0.f, Hour), 24.f);
	UpdateLighting();
}

void AOCWorldEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAdvanceTime && DayLengthMinutes > 0.f)
	{
		TimeOfDay = FMath::Fmod(TimeOfDay + DeltaSeconds * 24.f / (DayLengthMinutes * 60.f), 24.f);
	}

	// Moving the sun invalidates shadow caches, so only nudge it a few times per second.
	LightingTimer -= DeltaSeconds;
	if (LightingTimer <= 0.f)
	{
		LightingTimer = 0.5f;
		UpdateLighting();
	}
}

void AOCWorldEnvironment::UpdateLighting()
{
	// 06:00 = sunrise (pitch 0), 12:00 = overhead (pitch -90), 18:00 = sunset.
	const float SunPitch = -((TimeOfDay - 6.f) / 12.f) * 180.f;
	Sun->SetWorldRotation(FRotator(SunPitch, 35.f, 0.f));
	Moon->SetWorldRotation(FRotator(-SunPitch, 215.f, 0.f));

	const float SunHeight = FMath::Sin(FMath::DegreesToRadians(-SunPitch));
	Sun->SetIntensity(10.f * FMath::Clamp(SunHeight * 4.f, 0.f, 1.f));
	Moon->SetIntensity(0.4f * FMath::Clamp(-SunHeight * 3.f, 0.f, 1.f));
	SkyLight->SetIntensity(SunHeight > 0.f ? 1.f : 2.f);
}
