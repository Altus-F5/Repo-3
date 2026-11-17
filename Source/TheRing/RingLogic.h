#pragma once

#include "CoreMinimal.h"
#include "RingTypes.h"

/**
 * Pure rules with no engine state, so they can be unit tested
 * (Private/Tests/RingLogicTests.cpp).
 */
namespace RingLogic
{
	/** A stage may only advance to the one directly after it. */
	inline bool CanAdvance(ERingStage From, ERingStage To)
	{
		return static_cast<int32>(To) == static_cast<int32>(From) + 1;
	}

	/** New hold distance after a wheel input, kept inside [Min, Max]. */
	inline float AdjustHoldDistance(float Current, float WheelDelta, float Step, float Min, float Max)
	{
		return FMath::Clamp(Current + WheelDelta * Step, Min, Max);
	}

	/** Distance of Point along a track that starts at Origin and runs along AxisDir, clamped to [0, MaxTravel]. */
	inline float ProjectOnTrack(const FVector& Point, const FVector& Origin, const FVector& AxisDir, float MaxTravel)
	{
		return FMath::Clamp(static_cast<float>(FVector::DotProduct(Point - Origin, AxisDir)), 0.f, MaxTravel);
	}

	/** Step Current toward Target by at most MaxDelta. */
	inline float MoveTowards(float Current, float Target, float MaxDelta)
	{
		const float Delta = Target - Current;
		if (FMath::Abs(Delta) <= MaxDelta)
		{
			return Target;
		}
		return Current + FMath::Sign(Delta) * MaxDelta;
	}

	/** True when Position is close enough to SnapPosition to seat there. Height counts half. */
	inline bool IsWithinSnap(const FVector& Position, const FVector& SnapPosition, float Radius)
	{
		FVector Delta = Position - SnapPosition;
		Delta.Z *= 0.5;
		return Delta.SizeSquared() <= static_cast<double>(Radius) * Radius;
	}

	/** Seconds a subtitle stays up when the line gives no duration. */
	inline float SubtitleDuration(int32 CharacterCount)
	{
		return FMath::Max(2.2f, 1.2f + 0.055f * static_cast<float>(CharacterCount));
	}

	/** Smooth 0..1 ease. */
	inline float Ease(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return A * A * (3.f - 2.f * A);
	}

	/** 0 before Start, 1 after End, eased between. */
	inline float Ramp(float Time, float Start, float End)
	{
		if (End <= Start)
		{
			return Time >= End ? 1.f : 0.f;
		}
		return Ease((Time - Start) / (End - Start));
	}
}
