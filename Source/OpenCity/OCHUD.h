#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OCHUD.generated.h"

class UFont;

/** Canvas-drawn HUD, so it works with zero UMG assets. */
UCLASS(Config = Game)
class OPENCITY_API AOCHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	static void ShowBanner(const UObject* WorldContext, const FString& Title, const FString& Subtitle, const FLinearColor& Color, float Duration = 4.f);
	static void ShowNotification(const UObject* WorldContext, const FString& Text, float Duration = 3.f);

	/** Optional font with Hangul glyphs, e.g. an imported Noto Sans KR font asset. */
	UPROPERTY(Config)
	FSoftObjectPath FontPath;

protected:
	virtual void BeginPlay() override;

private:
	void DrawWorldMarker();
	void DrawMinimap(APawn* Pawn);
	void DrawStatus(APawn* Pawn);
	void DrawWanted();
	void DrawObjectiveAndDialogue();
	void DrawPrompt(APawn* Pawn);
	void DrawCrosshair(APawn* Pawn);
	void DrawNotifications();
	void DrawBanner();

	void DrawStar(const FVector2D& Center, float Radius, const FLinearColor& Color);
	void DrawTriangle(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color);
	void DrawShadowedText(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale);
	void DrawCenteredText(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale);
	void DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color);
	static bool ClipSegment(FVector2D& A, FVector2D& B, const FVector2D& Min, const FVector2D& Max);
	float Now() const;

	UPROPERTY(Transient) TObjectPtr<UFont> HUDFont;
	float UIScale = 1.f;

	FString BannerTitle;
	FString BannerSubtitle;
	FLinearColor BannerColor = FLinearColor::White;
	float BannerStart = -100.f;
	float BannerEnd = -100.f;

	struct FNotification
	{
		FString Text;
		float EndTime = 0.f;
	};
	TArray<FNotification> Notifications;
};
