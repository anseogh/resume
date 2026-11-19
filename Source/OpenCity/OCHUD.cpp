#include "OCHUD.h"
#include "CanvasItem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OCCharacter.h"
#include "OCCityGenerator.h"
#include "OCGameMode.h"
#include "OCHealthComponent.h"
#include "OCNPCCharacter.h"
#include "OCPopulationSubsystem.h"
#include "OCStorySubsystem.h"
#include "OCText.h"
#include "OCVehicle.h"
#include "OCWantedSubsystem.h"
#include "OCWorldEnvironment.h"
#include "RenderUtils.h"

namespace
{
	const FLinearColor Gold(1.f, 0.78f, 0.18f);
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.8f);
}

void AOCHUD::BeginPlay()
{
	Super::BeginPlay();
	if (FontPath.IsValid())
	{
		HUDFont = Cast<UFont>(FontPath.TryLoad());
	}
	if (!HUDFont && GEngine)
	{
		HUDFont = GEngine->GetMediumFont();
	}
}

float AOCHUD::Now() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void AOCHUD::ShowBanner(const UObject* WorldContext, const FString& Title, const FString& Subtitle, const FLinearColor& Color, float Duration)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (AOCHUD* HUD = PC ? PC->GetHUD<AOCHUD>() : nullptr)
	{
		HUD->BannerTitle = Title;
		HUD->BannerSubtitle = Subtitle;
		HUD->BannerColor = Color;
		HUD->BannerStart = HUD->Now();
		HUD->BannerEnd = HUD->BannerStart + Duration;
	}
}

void AOCHUD::ShowNotification(const UObject* WorldContext, const FString& Text, float Duration)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (AOCHUD* HUD = PC ? PC->GetHUD<AOCHUD>() : nullptr)
	{
		HUD->Notifications.Add({ Text, HUD->Now() + Duration });
		if (HUD->Notifications.Num() > 4)
		{
			HUD->Notifications.RemoveAt(0);
		}
	}
}

void AOCHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	UIScale = Canvas->ClipY / 1080.f;
	APawn* Pawn = PlayerOwner->GetPawn();

	DrawWorldMarker();
	DrawMinimap(Pawn);
	DrawStatus(Pawn);
	DrawWanted();
	DrawObjectiveAndDialogue();
	DrawPrompt(Pawn);
	DrawCrosshair(Pawn);
	DrawNotifications();
	DrawBanner();
}

void AOCHUD::DrawShadowedText(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale)
{
	const float S = Scale * UIScale;
	DrawText(Text, Shadow, X + 2.f * UIScale, Y + 2.f * UIScale, HUDFont, S);
	DrawText(Text, Color, X, Y, HUDFont, S);
}

void AOCHUD::DrawCenteredText(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale)
{
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, HUDFont, Scale * UIScale);
	DrawShadowedText(Text, CenterX - W * 0.5f, Y, Color, Scale);
}

void AOCHUD::DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, W, H);
	const float Pad = 2.f * UIScale;
	DrawRect(Color * FLinearColor(0.35f, 0.35f, 0.35f, 1.f), X + Pad, Y + Pad, W - Pad * 2.f, H - Pad * 2.f);
	DrawRect(Color, X + Pad, Y + Pad, (W - Pad * 2.f) * FMath::Clamp(Fraction, 0.f, 1.f), H - Pad * 2.f);
}

void AOCHUD::DrawTriangle(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color)
{
	FCanvasTriangleItem Triangle(A, B, C, GWhiteTexture);
	Triangle.SetColor(Color);
	Triangle.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Triangle);
}

void AOCHUD::DrawStar(const FVector2D& Center, float Radius, const FLinearColor& Color)
{
	FVector2D Points[10];
	for (int32 i = 0; i < 10; ++i)
	{
		const float Angle = FMath::DegreesToRadians(-90.f + i * 36.f);
		const float R = (i % 2 == 0) ? Radius : Radius * 0.45f;
		Points[i] = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R;
	}
	for (int32 i = 0; i < 10; ++i)
	{
		DrawTriangle(Center, Points[i], Points[(i + 1) % 10], Color);
	}
}

bool AOCHUD::ClipSegment(FVector2D& A, FVector2D& B, const FVector2D& Min, const FVector2D& Max)
{
	// Liang-Barsky line clipping against an axis-aligned rectangle.
	const FVector2D D = B - A;
	float T0 = 0.f, T1 = 1.f;
	const float P[4] = { static_cast<float>(-D.X), static_cast<float>(D.X), static_cast<float>(-D.Y), static_cast<float>(D.Y) };
	const float Q[4] = { static_cast<float>(A.X - Min.X), static_cast<float>(Max.X - A.X), static_cast<float>(A.Y - Min.Y), static_cast<float>(Max.Y - A.Y) };
	for (int32 i = 0; i < 4; ++i)
	{
		if (FMath::IsNearlyZero(P[i]))
		{
			if (Q[i] < 0.f)
			{
				return false;
			}
			continue;
		}
		const float T = Q[i] / P[i];
		if (P[i] < 0.f)
		{
			T0 = FMath::Max(T0, T);
		}
		else
		{
			T1 = FMath::Min(T1, T);
		}
		if (T0 > T1)
		{
			return false;
		}
	}
	const FVector2D Start = A;
	A = Start + D * T0;
	B = Start + D * T1;
	return true;
}

void AOCHUD::DrawMinimap(APawn* Pawn)
{
	AOCGameMode* GM = AOCGameMode::Get(this);
	AOCCityGenerator* City = GM ? GM->GetCity() : nullptr;
	if (!Pawn || !City || !PlayerOwner->PlayerCameraManager)
	{
		return;
	}

	const float Size = 280.f * UIScale;
	const FVector2D Min(30.f * UIScale, Canvas->ClipY - Size - 30.f * UIScale);
	const FVector2D Max = Min + FVector2D(Size);
	const FVector2D Center = (Min + Max) * 0.5f;
	DrawRect(FLinearColor(0.02f, 0.05f, 0.03f, 0.7f), Min.X, Min.Y, Size, Size);

	const bool bInVehicle = Cast<AOCVehicle>(Pawn) != nullptr;
	const float Range = bInVehicle ? 16000.f : 9000.f;
	const float Scale = (Size * 0.5f) / Range;
	const FVector PlayerLoc = Pawn->GetActorLocation();
	const float CamYaw = PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw;
	const FVector2D Fwd(FMath::Cos(FMath::DegreesToRadians(CamYaw)), FMath::Sin(FMath::DegreesToRadians(CamYaw)));
	const FVector2D Right(-Fwd.Y, Fwd.X);

	auto ToMap = [&](const FVector& World)
	{
		const FVector2D D(World.X - PlayerLoc.X, World.Y - PlayerLoc.Y);
		return Center + FVector2D(FVector2D::DotProduct(D, Right), -FVector2D::DotProduct(D, Fwd)) * Scale;
	};

	const float E = City->GetExtent();
	const float RoadThickness = FMath::Max(2.f, City->RoadWidth * Scale);
	const FLinearColor RoadColor(0.45f, 0.47f, 0.5f, 0.9f);
	for (int32 i = 0; i <= City->GridSize; ++i)
	{
		const float C = i * City->GetPitch();
		FVector2D A = ToMap(FVector(C, 0.f, 0.f)), B = ToMap(FVector(C, E, 0.f));
		if (ClipSegment(A, B, Min, Max))
		{
			DrawLine(A.X, A.Y, B.X, B.Y, RoadColor, RoadThickness);
		}
		A = ToMap(FVector(0.f, C, 0.f));
		B = ToMap(FVector(E, C, 0.f));
		if (ClipSegment(A, B, Min, Max))
		{
			DrawLine(A.X, A.Y, B.X, B.Y, RoadColor, RoadThickness);
		}
	}

	auto ClampToMap = [&](FVector2D P)
	{
		const float Inset = 6.f * UIScale;
		P.X = FMath::Clamp(P.X, Min.X + Inset, Max.X - Inset);
		P.Y = FMath::Clamp(P.Y, Min.Y + Inset, Max.Y - Inset);
		return P;
	};

	if (const UOCPopulationSubsystem* Population = GetWorld()->GetSubsystem<UOCPopulationSubsystem>())
	{
		const float Blip = 4.f * UIScale;
		for (const TWeakObjectPtr<AOCNPCCharacter>& Weak : Population->GetNPCs())
		{
			const AOCNPCCharacter* NPC = Weak.Get();
			if (!NPC)
			{
				continue;
			}
			const FLinearColor Color = NPC->GetBlipColor();
			if (Color.A <= 0.f)
			{
				continue;
			}
			const FVector2D P = ClampToMap(ToMap(NPC->GetActorLocation()));
			DrawRect(Color, P.X - Blip, P.Y - Blip, Blip * 2.f, Blip * 2.f);
		}
	}

	if (const UOCStorySubsystem* Story = GetWorld()->GetSubsystem<UOCStorySubsystem>())
	{
		FVector Target;
		if (Story->GetObjectiveLocation(Target))
		{
			const FVector2D P = ClampToMap(ToMap(Target));
			const float R = 8.f * UIScale;
			DrawTriangle(P + FVector2D(0.f, -R), P + FVector2D(R, 0.f), P + FVector2D(0.f, R), Gold);
			DrawTriangle(P + FVector2D(0.f, -R), P + FVector2D(0.f, R), P + FVector2D(-R, 0.f), Gold);
		}
	}

	const float Rel = FMath::DegreesToRadians(Pawn->GetActorRotation().Yaw - CamYaw);
	const FVector2D Dir(FMath::Sin(Rel), -FMath::Cos(Rel));
	const FVector2D Side(-Dir.Y, Dir.X);
	const float A = 10.f * UIScale;
	DrawTriangle(Center + Dir * A, Center - Dir * A * 0.6f + Side * A * 0.7f, Center - Dir * A * 0.6f - Side * A * 0.7f, FLinearColor::White);

	const FLinearColor Border(1.f, 1.f, 1.f, 0.4f);
	DrawLine(Min.X, Min.Y, Max.X, Min.Y, Border, 2.f);
	DrawLine(Max.X, Min.Y, Max.X, Max.Y, Border, 2.f);
	DrawLine(Max.X, Max.Y, Min.X, Max.Y, Border, 2.f);
	DrawLine(Min.X, Max.Y, Min.X, Min.Y, Border, 2.f);
}

void AOCHUD::DrawStatus(APawn* Pawn)
{
	AOCVehicle* Vehicle = Cast<AOCVehicle>(Pawn);
	AOCCharacter* Character = Vehicle ? Vehicle->GetDriver() : Cast<AOCCharacter>(Pawn);

	const float MapSize = 280.f * UIScale;
	const float X = 30.f * UIScale;
	const float Y = Canvas->ClipY - MapSize - 30.f * UIScale - 22.f * UIScale;
	const float W = MapSize;
	const float H = 14.f * UIScale;

	if (Character && Character->GetHealth())
	{
		DrawBar(X, Y, W, H, Character->GetHealth()->GetHealthFraction(), FLinearColor(0.2f, 0.75f, 0.25f));
	}
	if (Vehicle)
	{
		DrawBar(X, Y - H - 4.f * UIScale, W, H, Vehicle->GetHealthFraction(), FLinearColor(0.9f, 0.5f, 0.1f));
		const FString Speed = FString::Printf(TEXT("%d %s"), FMath::RoundToInt(Vehicle->GetSpeedKmh()), OCText::Kmh);
		DrawShadowedText(Speed, Canvas->ClipX - 230.f * UIScale, Canvas->ClipY - 90.f * UIScale, FLinearColor::White, 2.2f);
	}

	const float RightX = Canvas->ClipX - 40.f * UIScale;
	if (const UOCStorySubsystem* Story = GetWorld()->GetSubsystem<UOCStorySubsystem>())
	{
		const FString Money = FString::Printf(TEXT("$%s"), *FString::FormatAsNumber(Story->GetMoney()));
		float TW = 0.f, TH = 0.f;
		GetTextSize(Money, TW, TH, HUDFont, 2.f * UIScale);
		DrawShadowedText(Money, RightX - TW, 80.f * UIScale, FLinearColor(0.45f, 0.9f, 0.45f), 2.f);
	}

	AOCGameMode* GM = AOCGameMode::Get(this);
	if (AOCWorldEnvironment* Env = GM ? GM->GetEnvironment() : nullptr)
	{
		const float Time = Env->GetTimeOfDay();
		const FString Clock = FString::Printf(TEXT("%02d:%02d"), FMath::FloorToInt(Time), FMath::FloorToInt(FMath::Frac(Time) * 60.f));
		float TW = 0.f, TH = 0.f;
		GetTextSize(Clock, TW, TH, HUDFont, 1.4f * UIScale);
		DrawShadowedText(Clock, RightX - TW, 125.f * UIScale, FLinearColor::White, 1.4f);
	}
}

void AOCHUD::DrawWanted()
{
	const UOCWantedSubsystem* Wanted = GetWorld()->GetSubsystem<UOCWantedSubsystem>();
	if (!Wanted)
	{
		return;
	}
	const int32 Stars = Wanted->GetStars();
	const bool bBlinkOff = Wanted->IsSearching() && FMath::Fmod(Now(), 0.8f) < 0.4f;
	const float R = 16.f * UIScale;
	const float Spacing = 38.f * UIScale;
	const float StartX = Canvas->ClipX - 40.f * UIScale - R - 4.f * Spacing;
	const float Y = 50.f * UIScale;

	for (int32 i = 0; i < 5; ++i)
	{
		const FVector2D Center(StartX + i * Spacing, Y);
		const bool bLit = i < Stars && !bBlinkOff;
		DrawStar(Center + FVector2D(2.f, 2.f) * UIScale, R, Shadow);
		DrawStar(Center, R, bLit ? FLinearColor(1.f, 1.f, 1.f) : FLinearColor(0.25f, 0.25f, 0.25f, 0.7f));
	}
}

void AOCHUD::DrawObjectiveAndDialogue()
{
	const UOCStorySubsystem* Story = GetWorld()->GetSubsystem<UOCStorySubsystem>();
	if (!Story)
	{
		return;
	}
	const float CenterX = Canvas->ClipX * 0.5f;

	if (Story->IsMissionActive())
	{
		DrawShadowedText(Story->GetMissionTitle(), 40.f * UIScale, 40.f * UIScale, Gold, 1.6f);
	}

	const FString Objective = Story->GetObjectiveText();
	if (!Objective.IsEmpty())
	{
		DrawCenteredText(Objective, CenterX, Canvas->ClipY - 190.f * UIScale, FLinearColor::White, 1.6f);
	}

	FString Speaker, Line;
	if (Story->GetCurrentDialogue(Speaker, Line))
	{
		const FString Prefix = Speaker.IsEmpty() ? FString() : Speaker + TEXT(":  ");
		const float Scale = 1.5f * UIScale;
		float PW = 0.f, PH = 0.f, LW = 0.f, LH = 0.f;
		GetTextSize(Prefix, PW, PH, HUDFont, Scale);
		GetTextSize(Line, LW, LH, HUDFont, Scale);
		const float X = CenterX - (PW + LW) * 0.5f;
		const float Y = Canvas->ClipY - 140.f * UIScale;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), X - 16.f * UIScale, Y - 8.f * UIScale, PW + LW + 32.f * UIScale, FMath::Max(PH, LH) + 16.f * UIScale);
		DrawShadowedText(Prefix, X, Y, Gold, 1.5f);
		DrawShadowedText(Line, X + PW, Y, FLinearColor::White, 1.5f);
	}
}

void AOCHUD::DrawWorldMarker()
{
	const UOCStorySubsystem* Story = GetWorld()->GetSubsystem<UOCStorySubsystem>();
	const APawn* Pawn = PlayerOwner->GetPawn();
	FVector Target;
	if (!Story || !Pawn || !Story->GetObjectiveLocation(Target))
	{
		return;
	}

	FVector2D Screen;
	if (!PlayerOwner->ProjectWorldLocationToScreen(Target + FVector(0.f, 0.f, 250.f), Screen, false))
	{
		return;
	}
	const float R = 10.f * UIScale;
	DrawTriangle(Screen + FVector2D(0.f, -R), Screen + FVector2D(R, 0.f), Screen + FVector2D(0.f, R), Gold);
	DrawTriangle(Screen + FVector2D(0.f, -R), Screen + FVector2D(0.f, R), Screen + FVector2D(-R, 0.f), Gold);

	const int32 Meters = FMath::RoundToInt(FVector::Dist(Pawn->GetActorLocation(), Target) / 100.f);
	DrawCenteredText(FString::Printf(TEXT("%d%s"), Meters, OCText::Meters), Screen.X, Screen.Y + R + 4.f * UIScale, FLinearColor::White, 1.1f);
}

void AOCHUD::DrawPrompt(APawn* Pawn)
{
	FString Prompt;
	if (const AOCCharacter* Character = Cast<AOCCharacter>(Pawn))
	{
		if (!Character->IsDead() && Character->GetNearbyVehicle())
		{
			Prompt = OCText::EnterVehicle;
		}
	}
	else if (const AOCVehicle* Vehicle = Cast<AOCVehicle>(Pawn))
	{
		if (Vehicle->GetSpeedKmh() < 8.f)
		{
			Prompt = OCText::ExitVehicle;
		}
	}
	if (!Prompt.IsEmpty())
	{
		DrawCenteredText(Prompt, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.62f, FLinearColor::White, 1.4f);
	}
}

void AOCHUD::DrawCrosshair(APawn* Pawn)
{
	const AOCCharacter* Character = Cast<AOCCharacter>(Pawn);
	if (!Character || Character->IsDead())
	{
		return;
	}
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const float Gap = (Character->IsAiming() ? 4.f : 10.f) * UIScale;
	const float Len = 8.f * UIScale;
	const FLinearColor Color(1.f, 1.f, 1.f, Character->IsAiming() ? 0.95f : 0.5f);
	DrawLine(CX - Gap - Len, CY, CX - Gap, CY, Color, 2.f);
	DrawLine(CX + Gap, CY, CX + Gap + Len, CY, Color, 2.f);
	DrawLine(CX, CY - Gap - Len, CX, CY - Gap, Color, 2.f);
	DrawLine(CX, CY + Gap, CX, CY + Gap + Len, Color, 2.f);
}

void AOCHUD::DrawNotifications()
{
	const float T = Now();
	Notifications.RemoveAll([T](const FNotification& N) { return N.EndTime < T; });
	float Y = 90.f * UIScale;
	for (const FNotification& N : Notifications)
	{
		const float W = 460.f * UIScale;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 30.f * UIScale, Y - 6.f * UIScale, W, 36.f * UIScale);
		DrawShadowedText(N.Text, 42.f * UIScale, Y, FLinearColor::White, 1.25f);
		Y += 44.f * UIScale;
	}
}

void AOCHUD::DrawBanner()
{
	const float T = Now();
	if (T > BannerEnd || BannerTitle.IsEmpty())
	{
		return;
	}
	const float FadeIn = FMath::Clamp((T - BannerStart) / 0.3f, 0.f, 1.f);
	const float FadeOut = FMath::Clamp((BannerEnd - T) / 0.6f, 0.f, 1.f);
	const float Alpha = FMath::Min(FadeIn, FadeOut);

	FLinearColor TitleColor = BannerColor;
	TitleColor.A = Alpha;
	const float CX = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY * 0.3f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.4f * Alpha), 0.f, Y - 20.f * UIScale, Canvas->ClipX, 150.f * UIScale);
	DrawCenteredText(BannerTitle, CX, Y, TitleColor, 3.5f);
	if (!BannerSubtitle.IsEmpty())
	{
		DrawCenteredText(BannerSubtitle, CX, Y + 80.f * UIScale, FLinearColor(1.f, 1.f, 1.f, Alpha), 1.5f);
	}
}
