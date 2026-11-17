#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RingCharacter.generated.h"

class ARingTKObject;
class UCameraComponent;
class URingBodyComponent;
class URingSynthComponent;
class USpotLightComponent;
class USpringArmComponent;

/**
 * Elysia: third-person movement, the follow camera, and telekinesis.
 *
 * Telekinesis is one reusable interaction: focus (over-shoulder aim), take hold
 * of a marked object in range, carry it at a bounded distance while walking,
 * release. The character only chooses the hold point; ARingTKObject does the
 * bounded, collision-checked motion.
 *
 * Her visible body is the PLACEHOLDER figure in URingBodyComponent.
 */
UCLASS()
class THERING_API ARingCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARingCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Movement input: X right, Y forward, relative to the camera. */
	void MoveInput(const FVector2D& Axis);
	void SetBrisk(bool bInBrisk) { bBrisk = bInBrisk; }
	/** The focus (aim) input state. Holding an object keeps the focus framing regardless. */
	void SetFocusInput(bool bInFocus) { bFocusInput = bInFocus; }
	bool GetFocusInput() const { return bFocusInput; }
	/** Takes hold of the targeted object, or releases the held one. */
	void ToggleHold();
	/** Releases whatever is held. Safe to call at any time. */
	void ForceRelease();
	/** Wheel input: moves the held object nearer (negative) or farther (positive), within bounds. */
	void AdjustHoldDistance(float Delta);
	/** Moves the camera to the other shoulder. */
	void SwapShoulder() { ShoulderSide = -ShoulderSide; }

	void SetReducedMotion(bool bReduced);
	/** Exposure offset in stops applied on top of the scene's own. */
	void SetExposureBias(float Stops);
	void SetSuitLight(bool bOn) { bSuitLightOn = bOn; }
	void SetDroneLit(bool bLit);

	bool IsFocusing() const { return bFocusInput || HeldObject != nullptr; }
	ARingTKObject* GetHeldObject() const { return HeldObject; }
	/** The marked object under the reticle while focusing, if any. */
	ARingTKObject* GetTargetObject() const { return TargetObject; }
	/** Why the targeted object cannot be taken hold of; empty when it can. */
	const FString& GetTargetBlockedReason() const { return TargetBlockedReason; }
	/** A short message after a failed attempt, for the HUD. Empty when there is none. */
	const FString& GetNotice() const { return Notice; }
	URingSynthComponent* GetSynth() const { return Synth; }
	UCameraComponent* GetCamera() const { return Camera; }

private:
	void UpdateTarget();
	void UpdateHeld(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds);
	void ShowNotice(const FString& Text);

	UPROPERTY(VisibleAnywhere, Category = "Ring") TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY(VisibleAnywhere, Category = "Ring") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "Ring") TObjectPtr<URingBodyComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Ring") TObjectPtr<USpotLightComponent> SuitLight;
	UPROPERTY(VisibleAnywhere, Category = "Ring") TObjectPtr<URingSynthComponent> Synth;

	UPROPERTY() TObjectPtr<ARingTKObject> HeldObject;
	UPROPERTY() TObjectPtr<ARingTKObject> TargetObject;

	FString TargetBlockedReason;
	FString Notice;
	float NoticeTime = 0.f;
	float HoldDistance = 260.f;
	float FocusAlpha = 0.f;
	float ShoulderSide = 1.f;
	float ShoulderSmooth = 1.f;
	float SuitLightLevel = 0.f;
	float ExposureBias = 0.f;
	bool bBrisk = false;
	bool bFocusInput = false;
	bool bReducedMotion = false;
	bool bSuitLightOn = false;
	int32 LastStepCount = 0;
	int32 LastImpactCount = 0;
};
