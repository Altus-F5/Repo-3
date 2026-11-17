#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RingBuild.h"
#include "RingTKObject.generated.h"

class UStaticMeshComponent;

/**
 * An object Elysia can move by telekinesis.
 *
 * Motion is kinematic and bounded on purpose: a held object eases toward the
 * hold point in small collision-checked steps, so it cannot gain energy, tunnel
 * through a wall or be flung. A released object drops straight down and rests.
 * One that leaves the playable space returns to its home position.
 *
 * Two kinds:
 *  - Free: carried anywhere within range. May have a snap position (the bridge panel).
 *  - Track: constrained to one straight track (the heavy partition).
 */
UCLASS()
class THERING_API ARingTKObject : public AActor
{
	GENERATED_BODY()

public:
	ARingTKObject();
	virtual void Tick(float DeltaSeconds) override;

	/** Sets the name shown to the player, the size in centimetres and the look. Call once after spawning. */
	void Configure(const FString& InDisplayName, const FVector& InSizeCm, const FRingSurface& Surface);
	/** Makes this a track object that travels MaxTravel centimetres along AxisDirection from where it is now. */
	void ConfigureTrack(const FVector& AxisDirection, float MaxTravel);
	/** Gives a free object a position it seats into when released nearby. */
	void ConfigureSnap(const FVector& InSnapLocation, float InSnapRadius);

	/** False, with a player-readable reason, when the object cannot be taken hold of right now. */
	bool CanAcquire(FString& OutReason) const;
	void Acquire();
	void Release();
	/** Where the holder wants the object's centre to be. */
	void SetHoldTarget(const FVector& WorldTarget) { HoldTarget = WorldTarget; }

	/** Returns the object to its home position, unheld and unseated. */
	void ResetToHome();
	/** Changes the home position. */
	void SetHome(const FVector& Location) { HomeLocation = Location; }
	/** Puts a free object down at Location at rest (used when restoring a checkpoint). */
	void PlaceResting(const FVector& Location);
	/** Seats a free object in its snap position immediately. */
	void PlaceSnapped();
	/** Sets a track object's travel, 0 closed .. 1 fully open, immediately. */
	void PlaceOnTrack(float Alpha);

	bool IsHeld() const { return bHeld; }
	bool IsTrack() const { return bTrack; }
	bool IsSnapped() const { return bSnapped; }
	bool HasSnap() const { return bHasSnap; }
	/** True while held close enough to the snap position that releasing will seat it. */
	bool IsNearSnap() const;
	/** Track travel, 0 closed .. 1 fully open. */
	float GetTrackAlpha() const { return TrackTravel > 0.f ? TrackDistance / TrackTravel : 0.f; }
	/** 0..1, how fast the object moved this frame relative to its top speed. */
	float GetMotionAmount() const { return MotionAmount; }
	/** Counts set-downs hard enough to hear; the holder plays a thud when it changes. */
	int32 GetImpactCount() const { return ImpactCount; }
	const FString& GetDisplayName() const { return DisplayName; }
	FVector GetSizeCm() const { return SizeCm; }
	FVector GetSnapLocation() const { return SnapLocation; }

	/** Set by the director: the track is blocked (the partition while the brace is in place). */
	bool bJammed = false;
	/** Shown when a jammed object is targeted. */
	FString JammedReason;
	/** Set by the director: the player is standing on this object, so it must not be lifted. */
	bool bPlayerStandingOn = false;

private:
	bool IsSpaceFree(const FVector& Location) const;
	bool WouldHitPawn(const FVector& Location) const;
	/** Moves by Delta in small steps, sliding along whatever blocks. Returns the distance actually moved. */
	float MoveChecked(const FVector& Delta);
	void TickHeldFree(float DeltaSeconds);
	void TickHeldTrack(float DeltaSeconds);
	void TickFalling(float DeltaSeconds);

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;

	FString DisplayName;
	FVector SizeCm = FVector(100.0);
	FVector HomeLocation = FVector::ZeroVector;
	FVector HoldTarget = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	bool bHeld = false;
	bool bResting = true;
	float MotionAmount = 0.f;
	int32 ImpactCount = 0;

	bool bTrack = false;
	FVector TrackOrigin = FVector::ZeroVector;
	FVector TrackAxis = FVector::ForwardVector;
	float TrackTravel = 0.f;
	float TrackDistance = 0.f;

	bool bHasSnap = false;
	bool bSnapped = false;
	FVector SnapLocation = FVector::ZeroVector;
	float SnapRadius = 130.f;
};
