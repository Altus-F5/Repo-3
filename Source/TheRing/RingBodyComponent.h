#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RingBodyComponent.generated.h"

class UStaticMeshComponent;

/** Which of the three human looks to build. */
UENUM()
enum class ERingBodyLook : uint8
{
	Elysia,
	SurveyLead,
	Engineer
};

/** How the figure moves when it is not being driven by a character. */
UENUM()
enum class ERingIdleStyle : uint8
{
	/** Limbs follow SetLocomotion / SetPowerPose (the player). */
	Driven,
	/** Standing at a console, hands working. */
	ConsoleWork,
	/** Standing, looking over equipment. */
	Inspecting
};

/**
 * PLACEHOLDER human figure: a jointed body assembled from basic shapes and
 * animated in code. It follows the concept sheets for proportion, palette and
 * equipment layout, and stands in until real character models exist. The
 * origin is at the feet, facing +X.
 */
UCLASS()
class THERING_API URingBodyComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URingBodyComponent();

	/** Creates the body parts. Call once, after the owning actor has begun play. */
	void Build(ERingBodyLook InLook);
	/** Ground speed in cm/s; drives the walk cycle. */
	void SetLocomotion(float SpeedCmPerSec) { Speed = SpeedCmPerSec; }
	/** 0..1: raises the right arm into the restrained telekinesis pose. */
	void SetPowerPose(float Alpha) { PowerTarget = FMath::Clamp(Alpha, 0.f, 1.f); }
	void SetIdleStyle(ERingIdleStyle Style) { IdleStyle = Style; }
	/** Shows or hides the docked relay drone's indicator. */
	void SetDroneLit(bool bLit);
	/** Number of footfalls so far; the owner plays a step sound when it changes. */
	int32 GetStepCount() const { return StepCount; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Hips;
	UPROPERTY() TObjectPtr<USceneComponent> Chest;
	UPROPERTY() TObjectPtr<USceneComponent> HeadPivot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Thigh[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Shin[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Boot[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> UpperArm[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Forearm[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Hand[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> DroneLight;

	ERingBodyLook Look = ERingBodyLook::Elysia;
	ERingIdleStyle IdleStyle = ERingIdleStyle::Driven;
	bool bBuilt = false;
	float Speed = 0.f;
	float MoveAlpha = 0.f;
	float Phase = 0.f;
	float Clock = 0.f;
	float PowerTarget = 0.f;
	float PowerAlpha = 0.f;
	int32 StepCount = 0;
};
