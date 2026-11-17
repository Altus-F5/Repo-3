#include "RingSynthComponent.h"

namespace
{
	const float TwoPi = 6.28318530718f;

	/** Advances a 0..1 phase and returns its sine. */
	inline float Osc(float& Phase, float Frequency, float Rate)
	{
		Phase += Frequency / Rate;
		if (Phase >= 1.f)
		{
			Phase -= 1.f;
		}
		return FMath::Sin(Phase * TwoPi);
	}
}

URingSynthComponent::URingSynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumChannels = 1;
	bAllowSpatialization = false;
	bIsUISound = true; // keeps the room tone alive behind the pause menu
}

void URingSynthComponent::SetVolumes(float Master, float Ambience, float Effects, float Voice)
{
	MasterVolume.store(FMath::Clamp(Master, 0.f, 1.f));
	AmbienceVolume.store(FMath::Clamp(Ambience, 0.f, 1.f));
	EffectsVolume.store(FMath::Clamp(Effects, 0.f, 1.f));
	VoiceVolume.store(FMath::Clamp(Voice, 0.f, 1.f));
}

bool URingSynthComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	Rate = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

float URingSynthComponent::NextNoise()
{
	NoiseSeed = NoiseSeed * 1664525u + 1013904223u;
	return static_cast<float>((NoiseSeed >> 9) & 0x7FFFFFu) / 4194304.f - 1.f;
}

int32 URingSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const float Master = MasterVolume.load();
	const float AmbVol = AmbienceVolume.load();
	const float FxVol = EffectsVolume.load();
	const float VoxVol = VoiceVolume.load();
	const float DarkTarget = Darkness.load();
	const float HumTarget = Hum.load();
	const float GrindTarget = Grind.load();
	const float MurmurTarget = Murmur.load();
	const float TensionTarget = Tension.load();

	const int32 Footsteps = FootstepCount.load();
	if (Footsteps != SeenFootsteps) { SeenFootsteps = Footsteps; FootEnv = 1.f; }
	const int32 Thuds = ThudCount.load();
	if (Thuds != SeenThuds) { SeenThuds = Thuds; ThudEnv = 1.f; PhaseE = 0.f; }
	const int32 Clicks = ClickCount.load();
	if (Clicks != SeenClicks) { SeenClicks = Clicks; ClickEnv = 1.f; }

	const float Dt = 1.f / Rate;
	const float Slow = 1.f - FMath::Exp(-Dt / 0.25f);   // parameter smoothing, 250 ms
	const float FootDecay = FMath::Exp(-Dt / 0.05f);
	const float ThudDecay = FMath::Exp(-Dt / 0.18f);
	const float ClickDecay = FMath::Exp(-Dt / 0.03f);

	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		DarkSmooth += (DarkTarget - DarkSmooth) * Slow * 0.25f;
		HumSmooth += (HumTarget - HumSmooth) * Slow;
		GrindSmooth += (GrindTarget - GrindSmooth) * Slow;
		MurmurSmooth += (MurmurTarget - MurmurSmooth) * Slow;
		TensionSmooth += (TensionTarget - TensionSmooth) * Slow * 0.3f;

		const float White = NextNoise();

		// Room tone: air noise, a faint equipment hum in the lit area, a deeper rumble in the dark.
		Brown += 0.02f * (White - Brown);
		BrownDeep += 0.004f * (White - BrownDeep);
		const float Lfo = 0.5f + 0.5f * Osc(LfoPhase, 0.07f, Rate);
		const float Lit = (Brown * 0.9f + Osc(PhaseA, 100.f, Rate) * 0.010f + Osc(PhaseB, 50.f, Rate) * 0.018f) * (1.f - DarkSmooth);
		const float Dark = (BrownDeep * 4.5f + Osc(PhaseC, 38.f, Rate) * 0.035f * Lfo) * DarkSmooth;
		const float Ambience = (Lit + Dark) * 0.5f;

		// Footstep: a short low-passed scuff.
		FootLow += 0.18f * (White - FootLow);
		const float Foot = FootLow * FootEnv * 0.55f;
		FootEnv *= FootDecay;

		// Set-down thud.
		const float Thud = (Osc(PhaseE, 55.f, Rate) * 0.7f + FootLow * 0.25f) * ThudEnv;
		ThudEnv *= ThudDecay;

		// Interface click.
		const float Click = Osc(PhaseF, 1200.f, Rate) * ClickEnv * 0.12f;
		ClickEnv *= ClickDecay;

		// Telekinesis hum: two close tones that beat slowly. Deliberately plain.
		const float HumTone = (Osc(PhaseD, 110.f, Rate) + 0.6f * Osc(PhaseG, 164.8f, Rate)) * HumSmooth * 0.05f;

		// Heavy grind.
		GrindLow += 0.08f * (White - GrindLow);
		const float GrindTone = GrindLow * (0.6f + 0.4f * FMath::Sin(PhaseA * TwoPi * 0.17f)) * GrindSmooth * 0.9f;

		// Tension: a low tone that only swells.
		const float TensionTone = Osc(PhaseH, 41.f, Rate) * TensionSmooth * 0.10f;

		// Murmur: a buzzing source through two wandering resonances, chopped into
		// uneven syllables. It is noise shaped like speech, not a language.
		SyllableTime += Dt;
		if (SyllableTime >= SyllableLength)
		{
			SyllableTime = 0.f;
			const float R1 = 0.5f * (NextNoise() + 1.f);
			const float R2 = 0.5f * (NextNoise() + 1.f);
			const float R3 = 0.5f * (NextNoise() + 1.f);
			bSyllableVoiced = R3 > 0.22f;
			SyllableLength = bSyllableVoiced ? 0.14f + 0.26f * R1 : 0.08f + 0.30f * R1;
			FormantATarget = 280.f + 420.f * R2;
			FormantBTarget = 900.f + 1000.f * R3;
		}
		FormantA += (FormantATarget - FormantA) * 0.0008f;
		FormantB += (FormantBTarget - FormantB) * 0.0008f;
		VoiceWander += 0.00002f * (White - VoiceWander);
		VoicePhase += (82.f * (1.f + 6.f * VoiceWander)) / Rate;
		if (VoicePhase >= 1.f)
		{
			VoicePhase -= 1.f;
		}
		const float Shape = FMath::Sin(3.14159265f * FMath::Clamp(SyllableTime / SyllableLength, 0.f, 1.f));
		const float Source = (2.f * VoicePhase - 1.f) * (bSyllableVoiced ? Shape * Shape : 0.f);
		const float FA = 2.f * FMath::Sin(3.14159265f * FormantA / Rate);
		const float FB = 2.f * FMath::Sin(3.14159265f * FormantB / Rate);
		LowA += FA * BandA;
		BandA += FA * (Source - LowA - 0.25f * BandA);
		LowB += FB * BandB;
		BandB += FB * (Source - LowB - 0.25f * BandB);
		const float MurmurTone = (BandA + 0.6f * BandB) * MurmurSmooth * 0.10f;

		const float Mix = Master * (Ambience * AmbVol + (Foot + Thud + Click + HumTone + GrindTone + TensionTone) * FxVol + MurmurTone * VoxVol);
		OutAudio[Index] = FMath::Clamp(Mix, -1.f, 1.f);
	}
	return NumSamples;
}
