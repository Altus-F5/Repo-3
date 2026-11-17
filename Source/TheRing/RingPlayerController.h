#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RingPlayerController.generated.h"

class ARingCharacter;
class ARingGameMode;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** Which menu is open. */
UENUM()
enum class ERingMenu : uint8
{
	None,
	Pause,
	Options,
	Controls,
	EndCard
};

/**
 * Input and menus.
 *
 * Uses Enhanced Input. The actions and the mapping context are created in code
 * (no input assets), so the bindings below are the single source of truth:
 *
 *   Move            WASD / left stick
 *   Look            mouse / right stick
 *   Brisk walk      Left Shift / left stick click (hold)
 *   Interact        E / gamepad bottom face button
 *   Focus (aim)     Right mouse / left trigger   (hold, or toggle in Options)
 *   Take / let go   Left mouse / right trigger
 *   Hold distance   Mouse wheel / shoulder buttons
 *   Swap shoulder   Q / right stick click
 *   Pause           Escape, P / Start            (in the editor Escape ends Play, so use P)
 *   Menus           Arrows or WASD, Enter or Space, Backspace / D-pad, bottom and right face buttons
 *
 * Menu actions are the only ones that fire while paused.
 */
UCLASS()
class THERING_API ARingPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARingPlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Pauses and shows the end card. Called by the director when the demo ends. */
	void ShowEndCard();
	/** Pushes the saved options to the character, audio and engine. */
	void ApplySettings();

	ERingMenu GetMenu() const { return Menu; }
	/** What the HUD draws for the open menu. */
	void GetMenuView(FString& OutTitle, TArray<FString>& OutLines, int32& OutSelected, FString& OutFooter) const;

private:
	void EnsureInputAssets();
	ARingCharacter* GetRingCharacter() const;
	ARingGameMode* GetRingGameMode() const;
	void OpenMenu(ERingMenu NewMenu);
	void CloseMenus();
	int32 GetMenuItemCount() const;
	void AdjustOption(int32 Direction);
	void PlayClick();

	void OnMove(const FInputActionValue& Value);
	void OnLookMouse(const FInputActionValue& Value);
	void OnLookStick(const FInputActionValue& Value);
	void OnBriskStarted(const FInputActionValue& Value);
	void OnBriskCompleted(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);
	void OnFocusStarted(const FInputActionValue& Value);
	void OnFocusCompleted(const FInputActionValue& Value);
	void OnHold(const FInputActionValue& Value);
	void OnWheel(const FInputActionValue& Value);
	void OnNearer(const FInputActionValue& Value);
	void OnFarther(const FInputActionValue& Value);
	void OnShoulder(const FInputActionValue& Value);
	void OnPause(const FInputActionValue& Value);
	void OnMenuUp(const FInputActionValue& Value);
	void OnMenuDown(const FInputActionValue& Value);
	void OnMenuLeft(const FInputActionValue& Value);
	void OnMenuRight(const FInputActionValue& Value);
	void OnMenuConfirm(const FInputActionValue& Value);
	void OnMenuBack(const FInputActionValue& Value);

	void ApplyLook(float YawDegrees, float PitchDegrees);

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookMouseAction;
	UPROPERTY() TObjectPtr<UInputAction> LookStickAction;
	UPROPERTY() TObjectPtr<UInputAction> BriskAction;
	UPROPERTY() TObjectPtr<UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<UInputAction> FocusAction;
	UPROPERTY() TObjectPtr<UInputAction> HoldAction;
	UPROPERTY() TObjectPtr<UInputAction> WheelAction;
	UPROPERTY() TObjectPtr<UInputAction> NearerAction;
	UPROPERTY() TObjectPtr<UInputAction> FartherAction;
	UPROPERTY() TObjectPtr<UInputAction> ShoulderAction;
	UPROPERTY() TObjectPtr<UInputAction> PauseAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuUpAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuDownAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuLeftAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuRightAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuConfirmAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuBackAction;

	ERingMenu Menu = ERingMenu::None;
	int32 MenuIndex = 0;
	bool bInputBound = false;
};
