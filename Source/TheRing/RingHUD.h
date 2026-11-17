#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RingHUD.generated.h"

class UFont;

/**
 * Everything on screen, drawn on the canvas: objective, context prompt,
 * subtitles, telekinesis reticle and target brackets, the support-position
 * marker, the fade, and the menus. No widget assets.
 *
 * Nothing here relies on colour alone: targets get brackets and a name, the
 * selected menu row gets a marker and a frame, and every sound that matters
 * gets a caption.
 */
UCLASS()
class THERING_API ARingHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	FVector2D Measure(const FString& Text, float Scale) const;
	void Label(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color);
	void LabelCentred(const FString& Text, float CentreX, float Y, float Scale, const FLinearColor& Color);
	void Wrap(const FString& Text, float MaxWidth, float Scale, TArray<FString>& OutLines) const;
	void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness);
	void Brackets(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness);
	bool ScreenBounds(const AActor* Actor, float& OutX, float& OutY, float& OutW, float& OutH) const;

	void DrawGame(float W, float H, float UI);
	void DrawMenu(float W, float H, float UI);

	UPROPERTY() TObjectPtr<UFont> Font;
};
