#include "RingPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameUserSettings.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RingCharacter.h"
#include "RingGameInstance.h"
#include "RingGameMode.h"
#include "RingSynthComponent.h"
#include "RingTuning.h"
#include "TheRing.h"

namespace
{
	// Options menu rows.
	enum ERingOption
	{
		OptSubtitles,
		OptSubtitleSize,
		OptMaster,
		OptAmbience,
		OptEffects,
		OptVoice,
		OptSensitivity,
		OptInvertY,
		OptReducedMotion,
		OptBrightness,
		OptAimToggle,
		OptQuality,
		OptBack,
		OptCount
	};

	// Pause menu rows.
	enum ERingPauseItem
	{
		PauseResume,
		PauseCheckpoint,
		PauseResetPuzzle,
		PauseOptions,
		PauseControls,
		PauseRestartDemo,
		PauseQuit,
		PauseCount
	};

	const float VolumeStep = 0.1f;
	const float SensitivityStep = 0.1f;
	const float SensitivityMin = 0.2f;
	const float SensitivityMax = 3.f;
	const float BrightnessStep = 0.25f;
	const float BrightnessLimit = 2.f;
	/** Shoulder-button hold-distance speed, in wheel steps per second. */
	const float ButtonDistanceRate = 6.f;

	FString Meter(float Value01)
	{
		const int32 Filled = FMath::Clamp(FMath::RoundToInt(Value01 * 10.f), 0, 10);
		FString Out = TEXT("[");
		for (int32 Index = 0; Index < 10; ++Index)
		{
			Out += Index < Filled ? TEXT("#") : TEXT("-");
		}
		Out += FString::Printf(TEXT("] %d%%"), FMath::RoundToInt(Value01 * 100.f));
		return Out;
	}

	const TCHAR* OnOff(bool bValue) { return bValue ? TEXT("On") : TEXT("Off"); }
}

ARingPlayerController::ARingPlayerController()
{
	bShowMouseCursor = false;
}

ARingCharacter* ARingPlayerController::GetRingCharacter() const
{
	return Cast<ARingCharacter>(GetPawn());
}

ARingGameMode* ARingPlayerController::GetRingGameMode() const
{
	const UWorld* World = GetWorld();
	return World ? Cast<ARingGameMode>(World->GetAuthGameMode()) : nullptr;
}

void ARingPlayerController::EnsureInputAssets()
{
	if (Mapping)
	{
		return;
	}
	Mapping = NewObject<UInputMappingContext>(this, TEXT("RingMapping"));

	auto Make = [this](const TCHAR* Name, EInputActionValueType Type, bool bWhenPaused) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
		Action->ValueType = Type;
		Action->bTriggerWhenPaused = bWhenPaused;
		// Several actions share a key on purpose (W moves and also navigates menus);
		// which one acts is decided by whether a menu is open.
		Action->bConsumeInput = false;
		return Action;
	};
	auto Map = [this](UInputAction* Action, const FKey& Key, bool bNegate = false, bool bSwizzle = false, bool bDeadZone = false)
	{
		FEnhancedActionKeyMapping& Entry = Mapping->MapKey(Action, Key);
		if (bDeadZone)
		{
			Entry.Modifiers.Add(NewObject<UInputModifierDeadZone>(Mapping));
		}
		if (bSwizzle)
		{
			Entry.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
		}
		if (bNegate)
		{
			Entry.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
		}
	};

	MoveAction = Make(TEXT("RingMove"), EInputActionValueType::Axis2D, false);
	LookMouseAction = Make(TEXT("RingLookMouse"), EInputActionValueType::Axis2D, false);
	LookStickAction = Make(TEXT("RingLookStick"), EInputActionValueType::Axis2D, false);
	BriskAction = Make(TEXT("RingBrisk"), EInputActionValueType::Boolean, false);
	InteractAction = Make(TEXT("RingInteract"), EInputActionValueType::Boolean, false);
	FocusAction = Make(TEXT("RingFocus"), EInputActionValueType::Boolean, false);
	HoldAction = Make(TEXT("RingHold"), EInputActionValueType::Boolean, false);
	WheelAction = Make(TEXT("RingWheel"), EInputActionValueType::Axis1D, false);
	NearerAction = Make(TEXT("RingNearer"), EInputActionValueType::Boolean, false);
	FartherAction = Make(TEXT("RingFarther"), EInputActionValueType::Boolean, false);
	ShoulderAction = Make(TEXT("RingShoulder"), EInputActionValueType::Boolean, false);
	PauseAction = Make(TEXT("RingPause"), EInputActionValueType::Boolean, true);
	MenuUpAction = Make(TEXT("RingMenuUp"), EInputActionValueType::Boolean, true);
	MenuDownAction = Make(TEXT("RingMenuDown"), EInputActionValueType::Boolean, true);
	MenuLeftAction = Make(TEXT("RingMenuLeft"), EInputActionValueType::Boolean, true);
	MenuRightAction = Make(TEXT("RingMenuRight"), EInputActionValueType::Boolean, true);
	MenuConfirmAction = Make(TEXT("RingMenuConfirm"), EInputActionValueType::Boolean, true);
	MenuBackAction = Make(TEXT("RingMenuBack"), EInputActionValueType::Boolean, true);

	// Movement: X is right, Y is forward.
	Map(MoveAction, EKeys::W, false, true);
	Map(MoveAction, EKeys::S, true, true);
	Map(MoveAction, EKeys::D);
	Map(MoveAction, EKeys::A, true);
	Map(MoveAction, EKeys::Gamepad_Left2D, false, false, true);

	Map(LookMouseAction, EKeys::Mouse2D);
	Map(LookStickAction, EKeys::Gamepad_Right2D, false, false, true);

	Map(BriskAction, EKeys::LeftShift);
	Map(BriskAction, EKeys::Gamepad_LeftThumbstick);
	Map(InteractAction, EKeys::E);
	Map(InteractAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(FocusAction, EKeys::RightMouseButton);
	Map(FocusAction, EKeys::Gamepad_LeftTrigger);
	Map(HoldAction, EKeys::LeftMouseButton);
	Map(HoldAction, EKeys::Gamepad_RightTrigger);
	Map(WheelAction, EKeys::MouseWheelAxis);
	Map(NearerAction, EKeys::Gamepad_LeftShoulder);
	Map(FartherAction, EKeys::Gamepad_RightShoulder);
	Map(ShoulderAction, EKeys::Q);
	Map(ShoulderAction, EKeys::Gamepad_RightThumbstick);

	Map(PauseAction, EKeys::Escape);
	Map(PauseAction, EKeys::P);
	Map(PauseAction, EKeys::Gamepad_Special_Right);

	Map(MenuUpAction, EKeys::Up);
	Map(MenuUpAction, EKeys::W);
	Map(MenuUpAction, EKeys::Gamepad_DPad_Up);
	Map(MenuDownAction, EKeys::Down);
	Map(MenuDownAction, EKeys::S);
	Map(MenuDownAction, EKeys::Gamepad_DPad_Down);
	Map(MenuLeftAction, EKeys::Left);
	Map(MenuLeftAction, EKeys::A);
	Map(MenuLeftAction, EKeys::Gamepad_DPad_Left);
	Map(MenuRightAction, EKeys::Right);
	Map(MenuRightAction, EKeys::D);
	Map(MenuRightAction, EKeys::Gamepad_DPad_Right);
	Map(MenuConfirmAction, EKeys::Enter);
	Map(MenuConfirmAction, EKeys::SpaceBar);
	Map(MenuConfirmAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(MenuBackAction, EKeys::BackSpace);
	Map(MenuBackAction, EKeys::Gamepad_FaceButton_Right);
}

void ARingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	EnsureInputAssets();
	if (bInputBound)
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogTheRing, Error, TEXT("Enhanced Input is not the active input component. Check Config/DefaultInput.ini and that the EnhancedInput plugin is enabled."));
		return;
	}
	bInputBound = true;

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARingPlayerController::OnMove);
	Input->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ARingPlayerController::OnLookMouse);
	Input->BindAction(LookStickAction, ETriggerEvent::Triggered, this, &ARingPlayerController::OnLookStick);
	Input->BindAction(BriskAction, ETriggerEvent::Started, this, &ARingPlayerController::OnBriskStarted);
	Input->BindAction(BriskAction, ETriggerEvent::Completed, this, &ARingPlayerController::OnBriskCompleted);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ARingPlayerController::OnInteract);
	Input->BindAction(FocusAction, ETriggerEvent::Started, this, &ARingPlayerController::OnFocusStarted);
	Input->BindAction(FocusAction, ETriggerEvent::Completed, this, &ARingPlayerController::OnFocusCompleted);
	Input->BindAction(HoldAction, ETriggerEvent::Started, this, &ARingPlayerController::OnHold);
	Input->BindAction(WheelAction, ETriggerEvent::Triggered, this, &ARingPlayerController::OnWheel);
	Input->BindAction(NearerAction, ETriggerEvent::Triggered, this, &ARingPlayerController::OnNearer);
	Input->BindAction(FartherAction, ETriggerEvent::Triggered, this, &ARingPlayerController::OnFarther);
	Input->BindAction(ShoulderAction, ETriggerEvent::Started, this, &ARingPlayerController::OnShoulder);
	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ARingPlayerController::OnPause);
	Input->BindAction(MenuUpAction, ETriggerEvent::Started, this, &ARingPlayerController::OnMenuUp);
	Input->BindAction(MenuDownAction, ETriggerEvent::Started, this, &ARingPlayerController::OnMenuDown);
	Input->BindAction(MenuLeftAction, ETriggerEvent::Started, this, &ARingPlayerController::OnMenuLeft);
	Input->BindAction(MenuRightAction, ETriggerEvent::Started, this, &ARingPlayerController::OnMenuRight);
	Input->BindAction(MenuConfirmAction, ETriggerEvent::Started, this, &ARingPlayerController::OnMenuConfirm);
	Input->BindAction(MenuBackAction, ETriggerEvent::Started, this, &ARingPlayerController::OnMenuBack);
}

void ARingPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureInputAssets();
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	if (PlayerCameraManager)
	{
		const URingTuning& Tuning = URingTuning::Get();
		PlayerCameraManager->ViewPitchMin = Tuning.PitchMin;
		PlayerCameraManager->ViewPitchMax = Tuning.PitchMax;
	}
	ApplySettings();
}

void ARingPlayerController::ApplySettings()
{
	const URingGameInstance* Instance = GetGameInstance<URingGameInstance>();
	if (!Instance)
	{
		return;
	}
	const FRingSettings& Settings = Instance->Settings;
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->SetReducedMotion(Settings.bReducedMotion);
		if (URingSynthComponent* Synth = Character->GetSynth())
		{
			Synth->SetVolumes(Settings.MasterVolume, Settings.AmbienceVolume, Settings.EffectsVolume, Settings.VoiceVolume);
		}
	}
	if (GEngine)
	{
		if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
		{
			if (UserSettings->GetOverallScalabilityLevel() != Settings.Quality)
			{
				UserSettings->SetOverallScalabilityLevel(Settings.Quality);
				UserSettings->ApplyNonResolutionSettings();
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Gameplay input. None of these actions fire while the game is paused.
// ---------------------------------------------------------------------------

void ARingPlayerController::OnMove(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->MoveInput(Value.Get<FVector2D>());
	}
}

void ARingPlayerController::ApplyLook(float YawDegrees, float PitchDegrees)
{
	const URingTuning& Tuning = URingTuning::Get();
	bool bInvert = false;
	if (const URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		bInvert = Instance->Settings.bInvertY;
	}
	// The control rotation is set directly so the result does not depend on the
	// project's legacy input-scale settings: pushing up looks up unless inverted.
	FRotator Rotation = GetControlRotation();
	Rotation.Yaw += YawDegrees;
	Rotation.Pitch = FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(Rotation.Pitch)) + (bInvert ? -PitchDegrees : PitchDegrees), Tuning.PitchMin, Tuning.PitchMax);
	Rotation.Roll = 0.0;
	SetControlRotation(Rotation);
}

void ARingPlayerController::OnLookMouse(const FInputActionValue& Value)
{
	float Sensitivity = 1.f;
	if (const URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		Sensitivity = Instance->Settings.Sensitivity;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Scale = URingTuning::Get().MouseDegreesPerCount * Sensitivity;
	ApplyLook(static_cast<float>(Axis.X) * Scale, static_cast<float>(Axis.Y) * Scale);
}

void ARingPlayerController::OnLookStick(const FInputActionValue& Value)
{
	float Sensitivity = 1.f;
	if (const URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		Sensitivity = Instance->Settings.Sensitivity;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Scale = URingTuning::Get().StickDegreesPerSecond * Sensitivity * GetWorld()->GetDeltaSeconds();
	ApplyLook(static_cast<float>(Axis.X) * Scale, static_cast<float>(Axis.Y) * Scale);
}

void ARingPlayerController::OnBriskStarted(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->SetBrisk(true);
	}
}

void ARingPlayerController::OnBriskCompleted(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->SetBrisk(false);
	}
}

void ARingPlayerController::OnInteract(const FInputActionValue& Value)
{
	if (Menu != ERingMenu::None)
	{
		return;
	}
	if (ARingGameMode* GameMode = GetRingGameMode())
	{
		GameMode->HandleInteract();
	}
}

void ARingPlayerController::OnFocusStarted(const FInputActionValue& Value)
{
	ARingCharacter* Character = GetRingCharacter();
	const URingGameInstance* Instance = GetGameInstance<URingGameInstance>();
	if (!Character)
	{
		return;
	}
	const bool bToggle = Instance && Instance->Settings.bAimToggle;
	Character->SetFocusInput(bToggle ? !Character->GetFocusInput() : true);
}

void ARingPlayerController::OnFocusCompleted(const FInputActionValue& Value)
{
	ARingCharacter* Character = GetRingCharacter();
	const URingGameInstance* Instance = GetGameInstance<URingGameInstance>();
	if (Character && !(Instance && Instance->Settings.bAimToggle))
	{
		Character->SetFocusInput(false);
	}
}

void ARingPlayerController::OnHold(const FInputActionValue& Value)
{
	if (Menu != ERingMenu::None)
	{
		return;
	}
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->ToggleHold();
	}
}

void ARingPlayerController::OnWheel(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->AdjustHoldDistance(Value.Get<float>());
	}
}

void ARingPlayerController::OnNearer(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->AdjustHoldDistance(-ButtonDistanceRate * GetWorld()->GetDeltaSeconds());
	}
}

void ARingPlayerController::OnFarther(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->AdjustHoldDistance(ButtonDistanceRate * GetWorld()->GetDeltaSeconds());
	}
}

void ARingPlayerController::OnShoulder(const FInputActionValue& Value)
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->SwapShoulder();
	}
}

// ---------------------------------------------------------------------------
// Menus. These actions fire whether or not the game is paused; each handler
// checks which menu is open.
// ---------------------------------------------------------------------------

void ARingPlayerController::PlayClick()
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		if (URingSynthComponent* Synth = Character->GetSynth())
		{
			Synth->TriggerClick();
		}
	}
}

void ARingPlayerController::OpenMenu(ERingMenu NewMenu)
{
	Menu = NewMenu;
	MenuIndex = 0;
	UGameplayStatics::SetGamePaused(this, true);
}

void ARingPlayerController::CloseMenus()
{
	Menu = ERingMenu::None;
	MenuIndex = 0;
	if (URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		Instance->SaveToDisk();
		// A key released while paused is never reported, so clear held states.
		if (ARingCharacter* Character = GetRingCharacter())
		{
			Character->SetBrisk(false);
			if (!Instance->Settings.bAimToggle)
			{
				Character->SetFocusInput(false);
			}
		}
	}
	UGameplayStatics::SetGamePaused(this, false);
}

void ARingPlayerController::ShowEndCard()
{
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->ForceRelease();
	}
	OpenMenu(ERingMenu::EndCard);
}

int32 ARingPlayerController::GetMenuItemCount() const
{
	switch (Menu)
	{
	case ERingMenu::Pause: return PauseCount;
	case ERingMenu::Options: return OptCount;
	case ERingMenu::Controls: return 1;
	case ERingMenu::EndCard: return 2;
	default: return 0;
	}
}

void ARingPlayerController::OnPause(const FInputActionValue& Value)
{
	switch (Menu)
	{
	case ERingMenu::None:
	{
		const ARingGameMode* GameMode = GetRingGameMode();
		if (GameMode && GameMode->GetStage() != ERingStage::EndCard)
		{
			OpenMenu(ERingMenu::Pause);
		}
		break;
	}
	case ERingMenu::Pause:
		CloseMenus();
		break;
	case ERingMenu::Options:
	case ERingMenu::Controls:
		OpenMenu(ERingMenu::Pause);
		break;
	default:
		break; // the end card is left only by choosing an item
	}
}

void ARingPlayerController::OnMenuBack(const FInputActionValue& Value)
{
	if (Menu != ERingMenu::None)
	{
		OnPause(Value);
	}
}

void ARingPlayerController::OnMenuUp(const FInputActionValue& Value)
{
	const int32 Count = GetMenuItemCount();
	if (Count > 0)
	{
		MenuIndex = (MenuIndex + Count - 1) % Count;
		PlayClick();
	}
}

void ARingPlayerController::OnMenuDown(const FInputActionValue& Value)
{
	const int32 Count = GetMenuItemCount();
	if (Count > 0)
	{
		MenuIndex = (MenuIndex + 1) % Count;
		PlayClick();
	}
}

void ARingPlayerController::OnMenuLeft(const FInputActionValue& Value)
{
	if (Menu == ERingMenu::Options && MenuIndex != OptBack)
	{
		AdjustOption(-1);
	}
}

void ARingPlayerController::OnMenuRight(const FInputActionValue& Value)
{
	if (Menu == ERingMenu::Options && MenuIndex != OptBack)
	{
		AdjustOption(1);
	}
}

void ARingPlayerController::OnMenuConfirm(const FInputActionValue& Value)
{
	ARingGameMode* GameMode = GetRingGameMode();
	switch (Menu)
	{
	case ERingMenu::Pause:
		switch (MenuIndex)
		{
		case PauseResume: CloseMenus(); break;
		case PauseCheckpoint: if (GameMode) { GameMode->RestartFromCheckpoint(); } break;
		case PauseResetPuzzle: if (GameMode) { GameMode->ResetPuzzleObjects(); } CloseMenus(); break;
		case PauseOptions: OpenMenu(ERingMenu::Options); break;
		case PauseControls: OpenMenu(ERingMenu::Controls); break;
		case PauseRestartDemo: if (GameMode) { GameMode->RestartDemo(); } break;
		case PauseQuit: UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); break;
		default: break;
		}
		break;
	case ERingMenu::Options:
		if (MenuIndex == OptBack)
		{
			if (const URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
			{
				Instance->SaveToDisk();
			}
			OpenMenu(ERingMenu::Pause);
		}
		else
		{
			AdjustOption(1);
		}
		break;
	case ERingMenu::Controls:
		OpenMenu(ERingMenu::Pause);
		break;
	case ERingMenu::EndCard:
		if (MenuIndex == 0)
		{
			if (GameMode) { GameMode->RestartDemo(); }
		}
		else
		{
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		}
		break;
	default:
		break;
	}
}

void ARingPlayerController::AdjustOption(int32 Direction)
{
	URingGameInstance* Instance = GetGameInstance<URingGameInstance>();
	if (!Instance)
	{
		return;
	}
	FRingSettings& S = Instance->Settings;
	const float D = static_cast<float>(Direction);
	switch (MenuIndex)
	{
	case OptSubtitles: S.bSubtitles = !S.bSubtitles; break;
	case OptSubtitleSize: S.SubtitleSize = FMath::Clamp(S.SubtitleSize + Direction, 0, 2); break;
	case OptMaster: S.MasterVolume = FMath::Clamp(S.MasterVolume + D * VolumeStep, 0.f, 1.f); break;
	case OptAmbience: S.AmbienceVolume = FMath::Clamp(S.AmbienceVolume + D * VolumeStep, 0.f, 1.f); break;
	case OptEffects: S.EffectsVolume = FMath::Clamp(S.EffectsVolume + D * VolumeStep, 0.f, 1.f); break;
	case OptVoice: S.VoiceVolume = FMath::Clamp(S.VoiceVolume + D * VolumeStep, 0.f, 1.f); break;
	case OptSensitivity: S.Sensitivity = FMath::Clamp(S.Sensitivity + D * SensitivityStep, SensitivityMin, SensitivityMax); break;
	case OptInvertY: S.bInvertY = !S.bInvertY; break;
	case OptReducedMotion: S.bReducedMotion = !S.bReducedMotion; break;
	case OptBrightness: S.Brightness = FMath::Clamp(S.Brightness + D * BrightnessStep, -BrightnessLimit, BrightnessLimit); break;
	case OptAimToggle: S.bAimToggle = !S.bAimToggle; break;
	case OptQuality: S.Quality = FMath::Clamp(S.Quality + Direction, 0, 3); break;
	default: return;
	}
	ApplySettings();
	if (ARingCharacter* Character = GetRingCharacter())
	{
		Character->SetExposureBias(S.Brightness); // visible at once behind the menu
	}
	PlayClick();
}

void ARingPlayerController::GetMenuView(FString& OutTitle, TArray<FString>& OutLines, int32& OutSelected, FString& OutFooter) const
{
	OutLines.Reset();
	OutSelected = MenuIndex;
	OutFooter = TEXT("Up / Down: choose     Left / Right: change     Enter: select     P or Esc: back");
	const URingGameInstance* Instance = GetGameInstance<URingGameInstance>();
	const FRingSettings Settings = Instance ? Instance->Settings : FRingSettings();

	switch (Menu)
	{
	case ERingMenu::Pause:
		OutTitle = TEXT("PAUSED");
		OutLines.Add(TEXT("Resume"));
		OutLines.Add(FString(Instance && Instance->bCheckpointReached ? TEXT("Restart from checkpoint") : TEXT("Restart from checkpoint (none yet: restarts from the beginning)")));
		OutLines.Add(TEXT("Reset puzzle objects"));
		OutLines.Add(TEXT("Options"));
		OutLines.Add(TEXT("Controls"));
		OutLines.Add(TEXT("Restart demo from the beginning"));
		OutLines.Add(TEXT("Quit"));
		break;

	case ERingMenu::Options:
	{
		static const TCHAR* SizeNames[] = {TEXT("Small"), TEXT("Medium"), TEXT("Large")};
		static const TCHAR* QualityNames[] = {TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic")};
		OutTitle = TEXT("OPTIONS");
		OutLines.Add(FString::Printf(TEXT("Subtitles: %s"), OnOff(Settings.bSubtitles)));
		OutLines.Add(FString::Printf(TEXT("Subtitle size: %s"), SizeNames[FMath::Clamp(Settings.SubtitleSize, 0, 2)]));
		OutLines.Add(FString::Printf(TEXT("Master volume: %s"), *Meter(Settings.MasterVolume)));
		OutLines.Add(FString::Printf(TEXT("Ambience volume: %s"), *Meter(Settings.AmbienceVolume)));
		OutLines.Add(FString::Printf(TEXT("Effects volume: %s"), *Meter(Settings.EffectsVolume)));
		OutLines.Add(FString::Printf(TEXT("Voices volume: %s"), *Meter(Settings.VoiceVolume)));
		OutLines.Add(FString::Printf(TEXT("Camera sensitivity: %.1f"), Settings.Sensitivity));
		OutLines.Add(FString::Printf(TEXT("Invert vertical look: %s"), OnOff(Settings.bInvertY)));
		OutLines.Add(FString::Printf(TEXT("Reduced camera motion: %s"), OnOff(Settings.bReducedMotion)));
		OutLines.Add(FString::Printf(TEXT("Brightness: %+.2f stops"), Settings.Brightness));
		OutLines.Add(FString::Printf(TEXT("Focus button: %s"), Settings.bAimToggle ? TEXT("Toggle") : TEXT("Hold")));
		OutLines.Add(FString::Printf(TEXT("Graphics quality: %s"), QualityNames[FMath::Clamp(Settings.Quality, 0, 3)]));
		OutLines.Add(TEXT("Back"));
		break;
	}

	case ERingMenu::Controls:
		OutTitle = TEXT("CONTROLS");
		OutLines.Add(TEXT("Move: W A S D          Look: mouse"));
		OutLines.Add(TEXT("Brisk walk: hold Left Shift"));
		OutLines.Add(TEXT("Interact: E"));
		OutLines.Add(TEXT("Focus telekinesis: right mouse button"));
		OutLines.Add(TEXT("Take hold / let go: left mouse button"));
		OutLines.Add(TEXT("Held object nearer / farther: mouse wheel"));
		OutLines.Add(TEXT("Swap camera shoulder: Q"));
		OutLines.Add(TEXT("Pause: P or Escape"));
		OutLines.Add(TEXT("Gamepad: sticks move and look, LT focus, RT take / let go,"));
		OutLines.Add(TEXT("LB / RB distance, A interact, Start pause"));
		OutLines.Add(TEXT("Back"));
		OutSelected = OutLines.Num() - 1;
		break;

	case ERingMenu::EndCard:
		OutTitle = TEXT("THE RING");
		OutLines.Add(TEXT("Replay from the beginning"));
		OutLines.Add(TEXT("Quit"));
		OutFooter = TEXT("Working title.  End of the demo.     Up / Down: choose     Enter: select");
		break;

	default:
		OutTitle.Reset();
		OutFooter.Reset();
		break;
	}
}
