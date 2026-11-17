#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include <atomic>
#include "RingSynthComponent.generated.h"

/**
 * All of the demo's sound, generated in code: room tone, footsteps, the
 * telekinesis hum, the partition grinding, a set-down thud, an unintelligible
 * distant murmur and a low tension tone. PLACEHOLDER audio: it exists so the
 * demo is not silent without any imported sound assets.
 *
 * Setters are called from the game thread and read on the audio thread through
 * atomics; nothing else is shared.
 */
UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class THERING_API URingSynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	URingSynthComponent(const FObjectInitializer& ObjectInitializer);

	/** Mix levels, each 0..1. */
	void SetVolumes(float Master, float Ambience, float Effects, float Voice);
	/** 0 = lit staging area, 1 = unpowered sector. */
	void SetDarkness(float Value) { Darkness.store(FMath::Clamp(Value, 0.f, 1.f)); }
	/** Telekinesis hum, 0..1. */
	void SetHum(float Value) { Hum.store(FMath::Clamp(Value, 0.f, 1.f)); }
	/** Heavy object grinding on its track, 0..1. */
	void SetGrind(float Value) { Grind.store(FMath::Clamp(Value, 0.f, 1.f)); }
	/** Distant unintelligible speech, 0..1. */
	void SetMurmur(float Value) { Murmur.store(FMath::Clamp(Value, 0.f, 1.f)); }
	/** Low tension tone, 0..1. */
	void SetTension(float Value) { Tension.store(FMath::Clamp(Value, 0.f, 1.f)); }

	void TriggerFootstep() { FootstepCount.fetch_add(1); }
	void TriggerThud() { ThudCount.fetch_add(1); }
	void TriggerClick() { ClickCount.fetch_add(1); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	float NextNoise();

	std::atomic<float> MasterVolume{0.8f};
	std::atomic<float> AmbienceVolume{0.8f};
	std::atomic<float> EffectsVolume{0.8f};
	std::atomic<float> VoiceVolume{0.8f};
	std::atomic<float> Darkness{0.f};
	std::atomic<float> Hum{0.f};
	std::atomic<float> Grind{0.f};
	std::atomic<float> Murmur{0.f};
	std::atomic<float> Tension{0.f};
	std::atomic<int32> FootstepCount{0};
	std::atomic<int32> ThudCount{0};
	std::atomic<int32> ClickCount{0};

	// Audio-thread state.
	float Rate = 48000.f;
	uint32 NoiseSeed = 22222u;
	int32 SeenFootsteps = 0;
	int32 SeenThuds = 0;
	int32 SeenClicks = 0;
	float PhaseA = 0.f, PhaseB = 0.f, PhaseC = 0.f, PhaseD = 0.f, PhaseE = 0.f, PhaseF = 0.f, PhaseG = 0.f, PhaseH = 0.f;
	float LfoPhase = 0.f;
	float Brown = 0.f, BrownDeep = 0.f, FootLow = 0.f, GrindLow = 0.f;
	float FootEnv = 0.f, ThudEnv = 0.f, ClickEnv = 0.f;
	float DarkSmooth = 0.f, HumSmooth = 0.f, GrindSmooth = 0.f, MurmurSmooth = 0.f, TensionSmooth = 0.f;
	float VoicePhase = 0.f, VoiceWander = 0.f;
	float SyllableTime = 0.f, SyllableLength = 0.2f;
	bool bSyllableVoiced = false;
	float FormantA = 450.f, FormantB = 1300.f, FormantATarget = 450.f, FormantBTarget = 1300.f;
	float BandA = 0.f, LowA = 0.f, BandB = 0.f, LowB = 0.f;
};
