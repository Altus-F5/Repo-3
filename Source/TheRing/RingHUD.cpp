#include "RingHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "RingCharacter.h"
#include "RingGameInstance.h"
#include "RingGameMode.h"
#include "RingPlayerController.h"
#include "RingTKObject.h"

namespace
{
	const FLinearColor Ink(0.92f, 0.92f, 0.90f, 1.f);
	const FLinearColor Dim(0.70f, 0.70f, 0.68f, 1.f);
	const FLinearColor Brass(0.95f, 0.78f, 0.45f, 1.f);
	const FLinearColor Shade(0.f, 0.f, 0.f, 0.62f);
	const float SubtitleScales[] = {1.25f, 1.6f, 2.1f};
}

FVector2D ARingHUD::Measure(const FString& Text, float Scale) const
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Text, Width, Height, Font, Scale);
	return FVector2D(Width, Height);
}

void ARingHUD::Label(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color)
{
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A), X + 1.5f, Y + 1.5f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}

void ARingHUD::LabelCentred(const FString& Text, float CentreX, float Y, float Scale, const FLinearColor& Color)
{
	Label(Text, CentreX - static_cast<float>(Measure(Text, Scale).X) * 0.5f, Y, Scale, Color);
}

void ARingHUD::Wrap(const FString& Text, float MaxWidth, float Scale, TArray<FString>& OutLines) const
{
	OutLines.Reset();
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);
	FString Line;
	for (const FString& Word : Words)
	{
		const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		if (!Line.IsEmpty() && Measure(Candidate, Scale).X > MaxWidth)
		{
			OutLines.Add(Line);
			Line = Word;
		}
		else
		{
			Line = Candidate;
		}
	}
	if (!Line.IsEmpty())
	{
		OutLines.Add(Line);
	}
}

void ARingHUD::Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness)
{
	DrawLine(X, Y, X + W, Y, Color, Thickness);
	DrawLine(X + W, Y, X + W, Y + H, Color, Thickness);
	DrawLine(X + W, Y + H, X, Y + H, Color, Thickness);
	DrawLine(X, Y + H, X, Y, Color, Thickness);
}

void ARingHUD::Brackets(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness)
{
	const float Arm = FMath::Clamp(FMath::Min(W, H) * 0.25f, 8.f, 40.f);
	// Four corners.
	DrawLine(X, Y, X + Arm, Y, Color, Thickness);
	DrawLine(X, Y, X, Y + Arm, Color, Thickness);
	DrawLine(X + W, Y, X + W - Arm, Y, Color, Thickness);
	DrawLine(X + W, Y, X + W, Y + Arm, Color, Thickness);
	DrawLine(X, Y + H, X + Arm, Y + H, Color, Thickness);
	DrawLine(X, Y + H, X, Y + H - Arm, Color, Thickness);
	DrawLine(X + W, Y + H, X + W - Arm, Y + H, Color, Thickness);
	DrawLine(X + W, Y + H, X + W, Y + H - Arm, Color, Thickness);
}

bool ARingHUD::ScreenBounds(const AActor* Actor, float& OutX, float& OutY, float& OutW, float& OutH) const
{
	if (!Actor || !Canvas)
	{
		return false;
	}
	FVector Origin;
	FVector Extent;
	Actor->GetActorBounds(false, Origin, Extent);
	float MinX = TNumericLimits<float>::Max();
	float MinY = TNumericLimits<float>::Max();
	float MaxX = -TNumericLimits<float>::Max();
	float MaxY = -TNumericLimits<float>::Max();
	for (int32 Corner = 0; Corner < 8; ++Corner)
	{
		const FVector Point = Origin + FVector((Corner & 1) ? Extent.X : -Extent.X, (Corner & 2) ? Extent.Y : -Extent.Y, (Corner & 4) ? Extent.Z : -Extent.Z);
		const FVector Screen = Project(Point);
		if (Screen.Z <= 0.0)
		{
			return false; // partly behind the camera: skip rather than draw a wrong box
		}
		MinX = FMath::Min(MinX, static_cast<float>(Screen.X));
		MinY = FMath::Min(MinY, static_cast<float>(Screen.Y));
		MaxX = FMath::Max(MaxX, static_cast<float>(Screen.X));
		MaxY = FMath::Max(MaxY, static_cast<float>(Screen.Y));
	}
	OutX = FMath::Clamp(MinX, 0.f, Canvas->ClipX);
	OutY = FMath::Clamp(MinY, 0.f, Canvas->ClipY);
	OutW = FMath::Clamp(MaxX, 0.f, Canvas->ClipX) - OutX;
	OutH = FMath::Clamp(MaxY, 0.f, Canvas->ClipY) - OutY;
	return OutW > 2.f && OutH > 2.f;
}

void ARingHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	if (!Font && GEngine)
	{
		Font = GEngine->GetLargeFont();
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float UI = FMath::Max(0.6f, H / 1080.f);

	DrawGame(W, H, UI);
	DrawMenu(W, H, UI);
}

void ARingHUD::DrawGame(float W, float H, float UI)
{
	const UWorld* World = GetWorld();
	const ARingGameMode* GameMode = World ? Cast<ARingGameMode>(World->GetAuthGameMode()) : nullptr;
	const ARingCharacter* Character = Cast<ARingCharacter>(GetOwningPawn());
	const URingGameInstance* Instance = World ? Cast<URingGameInstance>(World->GetGameInstance()) : nullptr;
	if (!GameMode)
	{
		return;
	}
	const float Line = 2.f * UI;

	if (Character)
	{
		// Target brackets and name.
		const ARingTKObject* Target = Character->GetTargetObject();
		const ARingTKObject* Held = Character->GetHeldObject();
		float X = 0.f, Y = 0.f, BW = 0.f, BH = 0.f;
		if (Target && ScreenBounds(Target, X, Y, BW, BH))
		{
			const bool bBlocked = !Character->GetTargetBlockedReason().IsEmpty();
			Brackets(X - 6.f, Y - 6.f, BW + 12.f, BH + 12.f, bBlocked ? Dim : Brass, bBlocked ? Line * 0.6f : Line);
			LabelCentred(bBlocked ? Target->GetDisplayName().ToUpper() + TEXT("  (CANNOT MOVE)") : Target->GetDisplayName().ToUpper(),
				X + BW * 0.5f, FMath::Max(4.f, Y - 30.f * UI), 1.1f * UI, bBlocked ? Dim : Brass);
		}
		if (Held && ScreenBounds(Held, X, Y, BW, BH))
		{
			Brackets(X - 6.f, Y - 6.f, BW + 12.f, BH + 12.f, Ink, Line * 0.6f);
		}

		// Where the panel goes, while it is being carried.
		FVector SnapLocation;
		if (GameMode->GetSnapMarker(SnapLocation))
		{
			const FVector Screen = Project(SnapLocation);
			if (Screen.Z > 0.0)
			{
				const float SX = static_cast<float>(Screen.X);
				const float SY = static_cast<float>(Screen.Y);
				const float R = 14.f * UI;
				DrawLine(SX, SY - R, SX + R, SY, Brass, Line);
				DrawLine(SX + R, SY, SX, SY + R, Brass, Line);
				DrawLine(SX, SY + R, SX - R, SY, Brass, Line);
				DrawLine(SX - R, SY, SX, SY - R, Brass, Line);
				const bool bReady = Held && Held->IsNearSnap();
				LabelCentred(bReady ? TEXT("IN POSITION: LET GO TO SET") : TEXT("SUPPORT POSITION"), SX, SY + R + 6.f * UI, 1.1f * UI, bReady ? Ink : Brass);
			}
		}

		// Reticle: a small open ring while focusing; four ticks are added on a valid target.
		if (Character->IsFocusing())
		{
			const float CX = W * 0.5f;
			const float CY = H * 0.5f;
			const float R = 9.f * UI;
			const int32 Segments = 16;
			for (int32 Index = 0; Index < Segments; ++Index)
			{
				const float A0 = Index * 2.f * PI / Segments;
				const float A1 = (Index + 1) * 2.f * PI / Segments;
				DrawLine(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R, CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R, Ink, Line * 0.6f);
			}
			if (Held || (Target && Character->GetTargetBlockedReason().IsEmpty()))
			{
				const float T0 = R + 4.f * UI;
				const float T1 = R + 12.f * UI;
				DrawLine(CX - T1, CY, CX - T0, CY, Ink, Line);
				DrawLine(CX + T0, CY, CX + T1, CY, Ink, Line);
				DrawLine(CX, CY - T1, CX, CY - T0, Ink, Line);
				DrawLine(CX, CY + T0, CX, CY + T1, Ink, Line);
			}
		}
	}

	// Objective, top left.
	if (!GameMode->GetObjective().IsEmpty())
	{
		const float Scale = 1.25f * UI;
		const FVector2D Size = Measure(GameMode->GetObjective(), Scale);
		DrawRect(Shade, 28.f * UI, 26.f * UI, static_cast<float>(Size.X) + 28.f * UI, static_cast<float>(Size.Y) + 14.f * UI);
		DrawRect(Brass, 28.f * UI, 26.f * UI, 4.f * UI, static_cast<float>(Size.Y) + 14.f * UI);
		Label(GameMode->GetObjective(), 44.f * UI, 33.f * UI, Scale, Ink);
	}

	// Toast, top centre.
	if (!GameMode->GetToast().IsEmpty())
	{
		const float Scale = 1.25f * UI;
		const FVector2D Size = Measure(GameMode->GetToast(), Scale);
		DrawRect(Shade, W * 0.5f - static_cast<float>(Size.X) * 0.5f - 14.f * UI, 26.f * UI, static_cast<float>(Size.X) + 28.f * UI, static_cast<float>(Size.Y) + 14.f * UI);
		LabelCentred(GameMode->GetToast(), W * 0.5f, 33.f * UI, Scale, Ink);
	}

	// Subtitles, bottom centre. Captions for sounds are shown in square brackets.
	float SubtitleTop = H - 70.f * UI;
	FString Speaker;
	FString Text;
	const bool bSubtitles = !Instance || Instance->Settings.bSubtitles;
	if (bSubtitles && GameMode->GetSubtitle(Speaker, Text))
	{
		const int32 SizeIndex = Instance ? FMath::Clamp(Instance->Settings.SubtitleSize, 0, 2) : 1;
		const float Scale = SubtitleScales[SizeIndex] * UI;
		const FString Full = Speaker.IsEmpty() ? FString::Printf(TEXT("[%s]"), *Text) : FString::Printf(TEXT("%s:  %s"), *Speaker.ToUpper(), *Text);
		TArray<FString> Wrapped;
		Wrap(Full, W * 0.62f, Scale, Wrapped);
		const float LineHeight = static_cast<float>(Measure(TEXT("Ag"), Scale).Y) * 1.2f;
		float BoxWidth = 0.f;
		for (const FString& Row : Wrapped)
		{
			BoxWidth = FMath::Max(BoxWidth, static_cast<float>(Measure(Row, Scale).X));
		}
		const float BoxHeight = LineHeight * Wrapped.Num() + 16.f * UI;
		SubtitleTop = H - 64.f * UI - BoxHeight;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.72f), W * 0.5f - BoxWidth * 0.5f - 18.f * UI, SubtitleTop, BoxWidth + 36.f * UI, BoxHeight);
		for (int32 Index = 0; Index < Wrapped.Num(); ++Index)
		{
			LabelCentred(Wrapped[Index], W * 0.5f, SubtitleTop + 8.f * UI + LineHeight * Index, Scale, Ink);
		}
	}

	// Context prompt, above the subtitles.
	if (!GameMode->GetPrompt().IsEmpty())
	{
		const float Scale = 1.2f * UI;
		const FVector2D Size = Measure(GameMode->GetPrompt(), Scale);
		const float Y = SubtitleTop - static_cast<float>(Size.Y) - 26.f * UI;
		DrawRect(Shade, W * 0.5f - static_cast<float>(Size.X) * 0.5f - 14.f * UI, Y - 7.f * UI, static_cast<float>(Size.X) + 28.f * UI, static_cast<float>(Size.Y) + 14.f * UI);
		LabelCentred(GameMode->GetPrompt(), W * 0.5f, Y, Scale, Ink);
	}

	// Fade.
	const float Fade = GameMode->GetFade();
	if (Fade > 0.005f)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(Fade, 0.f, 1.f)), 0.f, 0.f, W, H);
	}
}

void ARingHUD::DrawMenu(float W, float H, float UI)
{
	const ARingPlayerController* Controller = Cast<ARingPlayerController>(GetOwningPlayerController());
	if (!Controller || Controller->GetMenu() == ERingMenu::None)
	{
		return;
	}
	FString Title;
	FString Footer;
	TArray<FString> Lines;
	int32 Selected = 0;
	Controller->GetMenuView(Title, Lines, Selected, Footer);

	const bool bEndCard = Controller->GetMenu() == ERingMenu::EndCard;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, bEndCard ? 1.f : 0.78f), 0.f, 0.f, W, H);

	const float TitleScale = (bEndCard ? 3.2f : 2.2f) * UI;
	const float RowScale = 1.4f * UI;
	const float RowHeight = static_cast<float>(Measure(TEXT("Ag"), RowScale).Y) * 1.55f;
	const float BlockHeight = RowHeight * Lines.Num();
	float Y = FMath::Max(40.f * UI, H * 0.5f - BlockHeight * 0.5f - 90.f * UI);

	LabelCentred(Title, W * 0.5f, Y, TitleScale, Ink);
	Y += static_cast<float>(Measure(Title, TitleScale).Y) + 12.f * UI;
	DrawLine(W * 0.5f - 160.f * UI, Y, W * 0.5f + 160.f * UI, Y, Brass, 2.f * UI);
	Y += 26.f * UI;

	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const bool bSelected = Index == Selected;
		// The selected row is marked three ways: a pointer, a frame, and brighter text.
		const FString Row = bSelected ? FString::Printf(TEXT(">  %s  <"), *Lines[Index]) : Lines[Index];
		const FVector2D Size = Measure(Row, RowScale);
		if (bSelected)
		{
			const float BoxX = W * 0.5f - static_cast<float>(Size.X) * 0.5f - 16.f * UI;
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.08f), BoxX, Y - 5.f * UI, static_cast<float>(Size.X) + 32.f * UI, static_cast<float>(Size.Y) + 10.f * UI);
			Frame(BoxX, Y - 5.f * UI, static_cast<float>(Size.X) + 32.f * UI, static_cast<float>(Size.Y) + 10.f * UI, Brass, 2.f * UI);
		}
		LabelCentred(Row, W * 0.5f, Y, RowScale, bSelected ? Ink : Dim);
		Y += RowHeight;
	}

	if (!Footer.IsEmpty())
	{
		LabelCentred(Footer, W * 0.5f, H - 60.f * UI, 1.0f * UI, Dim);
	}
}
