#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RingTuning.generated.h"

/**
 * Every tunable gameplay value in the demo. Values are read from
 * Config/DefaultGame.ini, section [/Script/TheRing.RingTuning]; the initialisers
 * below are only the fallback when a key is missing.
 */
UCLASS(Config = Game, defaultconfig)
class THERING_API URingTuning : public UObject
{
	GENERATED_BODY()

public:
	/** The tuning values as loaded from config. */
	static const URingTuning& Get() { return *GetDefault<URingTuning>(); }

	UPROPERTY(Config, EditAnywhere, Category = "Movement") float WalkSpeed = 230.f;
	UPROPERTY(Config, EditAnywhere, Category = "Movement") float BriskSpeed = 400.f;
	UPROPERTY(Config, EditAnywhere, Category = "Movement") float CarrySpeed = 200.f;

	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float TelekinesisRange = 900.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float HoldDistanceMin = 140.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float HoldDistanceMax = 520.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float HoldDistanceDefault = 260.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float HoldDistanceStep = 30.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float HeldMaxSpeed = 360.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float HeldStiffness = 6.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float SlideSpeed = 55.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float SnapRadius = 130.f;
	UPROPERTY(Config, EditAnywhere, Category = "Telekinesis") float ObjectKillZ = -250.f;

	UPROPERTY(Config, EditAnywhere, Category = "Interaction") float InteractRange = 230.f;

	UPROPERTY(Config, EditAnywhere, Category = "Camera") float CameraArmFree = 330.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float CameraArmFocus = 200.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float CameraShoulderOffset = 55.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float CameraFovFree = 80.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float CameraFovFocus = 70.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float PitchMin = -70.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float PitchMax = 60.f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float MouseDegreesPerCount = 0.07f;
	UPROPERTY(Config, EditAnywhere, Category = "Camera") float StickDegreesPerSecond = 140.f;

	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float ExposureMinEV = 0.5f;
	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float ExposureMaxEV = 6.f;
	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float ExposureBiasDark = -0.5f;
	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float StarLux = 60.f;
	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float WorkLightCandela = 260.f;
	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float SuitLightCandela = 400.f;
	UPROPERTY(Config, EditAnywhere, Category = "Lighting") float ChamberRimCandela = 500.f;

	UPROPERTY(Config, EditAnywhere, Category = "Encounter") float PrayDuration = 18.f;
};
