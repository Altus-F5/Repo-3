#pragma once

#include "CoreMinimal.h"
#include "RingTypes.generated.h"

/** Progression of the demo. Stages only ever advance by one (see RingLogic::CanAdvance). */
UENUM(BlueprintType)
enum class ERingStage : uint8
{
	Arrival,
	ClearBrace,
	OpenPartition,
	PlaceBridge,
	EnterCorridor,
	Approach,
	Encounter,
	Retreat,
	Ending,
	EndCard
};

/** Player options. Saved with URingSaveGame. */
USTRUCT(BlueprintType)
struct FRingSettings
{
	GENERATED_BODY()

	UPROPERTY() bool bSubtitles = true;
	/** 0 small, 1 medium, 2 large. */
	UPROPERTY() int32 SubtitleSize = 1;
	UPROPERTY() float MasterVolume = 0.8f;
	UPROPERTY() float AmbienceVolume = 0.8f;
	UPROPERTY() float EffectsVolume = 0.8f;
	UPROPERTY() float VoiceVolume = 0.8f;
	UPROPERTY() float Sensitivity = 1.0f;
	UPROPERTY() bool bInvertY = false;
	UPROPERTY() bool bReducedMotion = false;
	/** Exposure offset in stops, -2..+2. */
	UPROPERTY() float Brightness = 0.f;
	/** true: the focus button toggles. false: hold to focus. */
	UPROPERTY() bool bAimToggle = false;
	/** Engine scalability level 0..3. */
	UPROPERTY() int32 Quality = 2;
};
