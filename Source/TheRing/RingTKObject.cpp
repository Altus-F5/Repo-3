#include "RingTKObject.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RingDust.h"
#include "RingLogic.h"
#include "RingTuning.h"

namespace
{
	/** Largest single collision-checked step, cm. Smaller than the thinnest wall in the level. */
	const float MaxStep = 6.f;
	/** The collision box is this much smaller per side than the object, so resting contact is not a hit. */
	const float Skin = 1.f;
	const float Gravity = 980.f;
	/** A landing faster than this is audible and raises dust. */
	const float ImpactSpeed = 120.f;
}

ARingTKObject::ARingTKObject()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCanEverAffectNavigation(false);
}

void ARingTKObject::Configure(const FString& InDisplayName, const FVector& InSizeCm, const FRingSurface& Surface)
{
	DisplayName = InDisplayName;
	SizeCm = InSizeCm;
	HomeLocation = GetActorLocation();

	Mesh->SetStaticMesh(FRingBuild::Mesh(ERingShape::Cube));
	Mesh->SetRelativeScale3D(SizeCm / 100.0);
	if (UMaterialInstanceDynamic* Material = FRingBuild::MakeMaterial(Mesh, Surface))
	{
		Mesh->SetMaterial(0, Material);
	}
	// The camera boom must not be pushed around by something the player is carrying.
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Two brass bands mark the object as one that can be moved. The marking is a
	// shape, not only a colour.
	const FRingSurface Brass(FLinearColor(0.42f, 0.29f, 0.10f), 0.4f, 0.85f);
	int32 LongAxis = 0;
	if (SizeCm.Y > SizeCm[LongAxis]) { LongAxis = 1; }
	if (SizeCm.Z > SizeCm[LongAxis]) { LongAxis = 2; }
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		FVector Offset = FVector::ZeroVector;
		Offset[LongAxis] = SizeCm[LongAxis] * 0.36 * Side;
		FVector BandSize = SizeCm + FVector(1.6);
		BandSize[LongAxis] = FMath::Max(4.0, SizeCm[LongAxis] * 0.03);
		FRingBuild::AddPart(this, RootComponent, ERingShape::Cube, Offset, BandSize, Brass);
	}
}

void ARingTKObject::ConfigureTrack(const FVector& AxisDirection, float MaxTravel)
{
	bTrack = true;
	TrackOrigin = GetActorLocation();
	TrackAxis = AxisDirection.GetSafeNormal();
	TrackTravel = MaxTravel;
	TrackDistance = 0.f;
}

void ARingTKObject::ConfigureSnap(const FVector& InSnapLocation, float InSnapRadius)
{
	bHasSnap = true;
	SnapLocation = InSnapLocation;
	SnapRadius = InSnapRadius;
}

bool ARingTKObject::CanAcquire(FString& OutReason) const
{
	if (bJammed)
	{
		OutReason = JammedReason;
		return false;
	}
	if (bPlayerStandingOn)
	{
		OutReason = TEXT("Step off it first.");
		return false;
	}
	return true;
}

void ARingTKObject::Acquire()
{
	if (bHeld)
	{
		return;
	}
	bHeld = true;
	bResting = false;
	bSnapped = false;
	Velocity = FVector::ZeroVector;
	HoldTarget = GetActorLocation();
	if (!bTrack)
	{
		ARingDust::Puff(GetWorld(), GetActorLocation() - FVector(0.0, 0.0, SizeCm.Z * 0.5), FMath::Min(60.0, SizeCm.GetMax() * 0.3));
	}
}

void ARingTKObject::Release()
{
	if (!bHeld)
	{
		return;
	}
	bHeld = false;
	MotionAmount = 0.f;
	if (bTrack)
	{
		return; // a track object stays where it was left
	}
	if (bHasSnap && RingLogic::IsWithinSnap(GetActorLocation(), SnapLocation, SnapRadius))
	{
		PlaceSnapped();
		++ImpactCount;
		ARingDust::Puff(GetWorld(), SnapLocation, 50.f);
		return;
	}
	Velocity = FVector::ZeroVector;
	bResting = false;
}

void ARingTKObject::ResetToHome()
{
	bHeld = false;
	bSnapped = false;
	Velocity = FVector::ZeroVector;
	MotionAmount = 0.f;
	if (bTrack)
	{
		PlaceOnTrack(0.f);
		return;
	}
	SetActorLocation(HomeLocation);
	bResting = true;
}

void ARingTKObject::PlaceResting(const FVector& Location)
{
	bHeld = false;
	bSnapped = false;
	Velocity = FVector::ZeroVector;
	SetActorLocation(Location);
	bResting = true;
}

void ARingTKObject::PlaceSnapped()
{
	if (!bHasSnap)
	{
		return;
	}
	bHeld = false;
	bSnapped = true;
	bResting = true;
	Velocity = FVector::ZeroVector;
	SetActorLocation(SnapLocation);
}

void ARingTKObject::PlaceOnTrack(float Alpha)
{
	if (!bTrack)
	{
		return;
	}
	TrackDistance = FMath::Clamp(Alpha, 0.f, 1.f) * TrackTravel;
	SetActorLocation(TrackOrigin + TrackAxis * TrackDistance);
}

bool ARingTKObject::IsNearSnap() const
{
	return bHeld && bHasSnap && RingLogic::IsWithinSnap(GetActorLocation(), SnapLocation, SnapRadius);
}

bool ARingTKObject::IsSpaceFree(const FVector& Location) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FCollisionQueryParams Params(FName(TEXT("RingTKMove")), false, this);
	const FVector HalfExtent = (SizeCm * 0.5 - FVector(Skin)).ComponentMax(FVector(0.5));
	return !World->OverlapBlockingTestByChannel(Location, GetActorQuat(), ECC_WorldDynamic, FCollisionShape::MakeBox(HalfExtent), Params);
}

bool ARingTKObject::WouldHitPawn(const FVector& Location) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	FCollisionQueryParams Params(FName(TEXT("RingTKPawn")), false, this);
	return World->OverlapAnyTestByObjectType(Location, GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeBox(SizeCm * 0.5), Params);
}

float ARingTKObject::MoveChecked(const FVector& Delta)
{
	const float Distance = Delta.Size();
	if (Distance < KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Distance / MaxStep), 1, 16);
	const FVector Step = Delta / static_cast<double>(Steps);
	FVector Location = GetActorLocation();
	const FVector Start = Location;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		if (IsSpaceFree(Location + Step))
		{
			Location += Step;
			continue;
		}
		// Blocked: keep whatever part of the step each axis still allows, and lose the speed on the rest.
		bool bMovedAny = false;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			FVector AxisStep = FVector::ZeroVector;
			AxisStep[Axis] = Step[Axis];
			if (!FMath::IsNearlyZero(AxisStep[Axis]) && IsSpaceFree(Location + AxisStep))
			{
				Location += AxisStep;
				bMovedAny = true;
			}
			else
			{
				Velocity[Axis] = 0.0;
			}
		}
		if (!bMovedAny)
		{
			break;
		}
	}
	SetActorLocation(Location);
	return static_cast<float>((Location - Start).Size());
}

void ARingTKObject::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	if (bHeld)
	{
		if (bTrack)
		{
			TickHeldTrack(DeltaSeconds);
		}
		else
		{
			TickHeldFree(DeltaSeconds);
		}
	}
	else if (!bTrack && !bResting)
	{
		TickFalling(DeltaSeconds);
	}
	else
	{
		MotionAmount = 0.f;
	}

	if (!bTrack && GetActorLocation().Z < URingTuning::Get().ObjectKillZ)
	{
		// Lost below the floor or outside the hull: bring it back rather than strand the player.
		ResetToHome();
		ARingDust::Puff(GetWorld(), HomeLocation, 40.f);
	}
}

void ARingTKObject::TickHeldFree(float DeltaSeconds)
{
	const URingTuning& Tuning = URingTuning::Get();
	const FVector ToTarget = HoldTarget - GetActorLocation();
	const FVector Desired = (ToTarget * Tuning.HeldStiffness).GetClampedToMaxSize(Tuning.HeldMaxSpeed);
	Velocity = FMath::VInterpTo(Velocity, Desired, DeltaSeconds, 8.f);
	const float Moved = MoveChecked(Velocity * DeltaSeconds);
	MotionAmount = FMath::Clamp(Moved / FMath::Max(1.f, Tuning.HeldMaxSpeed * DeltaSeconds), 0.f, 1.f);

	// A slight lean into the motion, so the object reads as pushed rather than attached.
	const FRotator Lean(FMath::Clamp(static_cast<float>(-Velocity.X) * 0.012f, -4.f, 4.f), 0.f, FMath::Clamp(static_cast<float>(Velocity.Y) * 0.012f, -4.f, 4.f));
	Mesh->SetRelativeRotation(FMath::RInterpTo(Mesh->GetRelativeRotation(), Lean, DeltaSeconds, 6.f));
}

void ARingTKObject::TickHeldTrack(float DeltaSeconds)
{
	const URingTuning& Tuning = URingTuning::Get();
	MotionAmount = 0.f;
	if (bJammed)
	{
		return;
	}
	const float Wanted = RingLogic::ProjectOnTrack(HoldTarget, TrackOrigin, TrackAxis, TrackTravel);
	const float Speed = FMath::Min(Tuning.SlideSpeed, FMath::Abs(Wanted - TrackDistance) * 3.f);
	const float Next = RingLogic::MoveTowards(TrackDistance, Wanted, Speed * DeltaSeconds);
	if (FMath::IsNearlyEqual(Next, TrackDistance, 0.001f))
	{
		return;
	}
	const FVector NextLocation = TrackOrigin + TrackAxis * Next;
	if (WouldHitPawn(NextLocation))
	{
		return; // never close on someone standing in the doorway
	}
	TrackDistance = Next;
	SetActorLocation(NextLocation);
	MotionAmount = FMath::Clamp(Speed / FMath::Max(1.f, Tuning.SlideSpeed), 0.f, 1.f);
}

void ARingTKObject::TickFalling(float DeltaSeconds)
{
	Mesh->SetRelativeRotation(FMath::RInterpTo(Mesh->GetRelativeRotation(), FRotator::ZeroRotator, DeltaSeconds, 10.f));
	Velocity.X = 0.0;
	Velocity.Y = 0.0;
	Velocity.Z -= Gravity * DeltaSeconds;
	const float FallSpeed = static_cast<float>(-Velocity.Z);
	const float Wanted = FallSpeed * DeltaSeconds;
	const float Moved = MoveChecked(FVector(0.0, 0.0, -Wanted));
	MotionAmount = 0.f;
	if (Moved + 0.01f < Wanted)
	{
		bResting = true;
		Velocity = FVector::ZeroVector;
		Mesh->SetRelativeRotation(FRotator::ZeroRotator);
		if (FallSpeed > ImpactSpeed)
		{
			++ImpactCount;
			ARingDust::Puff(GetWorld(), GetActorLocation() - FVector(0.0, 0.0, SizeCm.Z * 0.5), FMath::Min(60.0, SizeCm.GetMax() * 0.3));
		}
	}
}
