#include "RingBodyComponent.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "RingBuild.h"

namespace
{
	// Proportions in centimetres for a 1.70 m figure. Index 0 is the left side (-Y), 1 the right (+Y).
	const float HipHeight = 90.f;
	const float HipHalfWidth = 8.5f;
	const float ThighLength = 44.f;
	const float ShinLength = 38.f;
	const float ShoulderHeight = 138.f;
	const float ShoulderHalfWidth = 19.5f;
	const float UpperArmLength = 28.f;
	const float ForearmLength = 25.f;
	const float StrideLength = 70.f;

	float SideSign(int32 Side) { return Side == 0 ? -1.f : 1.f; }
}

URingBodyComponent::URingBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URingBodyComponent::Build(ERingBodyLook InLook)
{
	AActor* Owner = GetOwner();
	if (bBuilt || !Owner)
	{
		return;
	}
	bBuilt = true;
	Look = InLook;
	const bool bElysia = Look == ERingBodyLook::Elysia;

	// Palette from the concept sheets.
	const FRingSurface Suit(bElysia ? FLinearColor(0.018f, 0.018f, 0.020f) : FLinearColor(0.045f, 0.045f, 0.050f), 0.85f, 0.f);
	const FRingSurface Strap(FLinearColor(0.070f, 0.055f, 0.040f), 0.7f, 0.f);
	const FRingSurface Brass(FLinearColor(0.38f, 0.26f, 0.09f), 0.45f, 0.8f);
	const FRingSurface Boots(FLinearColor(0.012f, 0.012f, 0.012f), 0.6f, 0.f);
	const FRingSurface Slate(FLinearColor(0.12f, 0.125f, 0.13f), 0.8f, 0.f);
	const FRingSurface SlateBlue(FLinearColor(0.10f, 0.18f, 0.32f), 0.6f, 0.1f);
	FRingSurface Skin(FLinearColor(0.55f, 0.36f, 0.27f), 0.6f, 0.f);
	FRingSurface Hair(FLinearColor(0.05f, 0.03f, 0.02f), 0.5f, 0.f);
	if (Look == ERingBodyLook::SurveyLead)
	{
		Skin.Color = FLinearColor(0.50f, 0.36f, 0.28f);
		Hair.Color = FLinearColor(0.33f, 0.33f, 0.34f);
	}
	else if (Look == ERingBodyLook::Engineer)
	{
		Skin.Color = FLinearColor(0.60f, 0.45f, 0.36f);
		Hair.Color = FLinearColor(0.015f, 0.015f, 0.018f);
	}

	Hips = FRingBuild::AddPivot(Owner, this, FVector::ZeroVector);
	Chest = FRingBuild::AddPivot(Owner, Hips, FVector(0.0, 0.0, 100.0));
	HeadPivot = FRingBuild::AddPivot(Owner, Chest, FVector(0.0, 0.0, 44.0));

	// Pelvis, belt and pouches.
	FRingBuild::AddPart(Owner, Hips, ERingShape::Cube, FVector(0, 0, 92), FVector(17, 29, 16), Suit);
	FRingBuild::AddPart(Owner, Hips, ERingShape::Cube, FVector(0, 0, 101), FVector(18.5, 30.5, 4.5), Strap);
	FRingBuild::AddPart(Owner, Hips, ERingShape::Cube, FVector(9.5, 0, 101), FVector(1.5, 5, 4), Brass);
	FRingBuild::AddPart(Owner, Hips, ERingShape::Cube, FVector(2, -16.5, 95), FVector(8, 5, 10), Strap);
	FRingBuild::AddPart(Owner, Hips, ERingShape::Cube, FVector(2, 16.5, 95), FVector(8, 5, 10), Strap);
	FRingBuild::AddPart(Owner, Hips, ERingShape::Cylinder, FVector(-4, 17.5, 88), FVector(4, 4, 14), Boots); // field tool

	// Torso.
	FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(0, 0, 21), FVector(18, 31, 40), Suit);
	FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(0, 0, 37), FVector(17, 38, 8), Suit);
	FRingBuild::AddPart(Owner, Chest, ERingShape::Cylinder, FVector(0, 0, 43), FVector(11, 11, 5), Suit);
	// Harness.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Y = 9.f * SideSign(Side);
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(9.3, Y, 22), FVector(0.8, 4.5, 36), Strap);
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(-9.3, Y, 22), FVector(0.8, 4.5, 36), Strap);
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(9.6, Y + 2.4f * SideSign(Side), 22), FVector(0.5, 0.7, 36), Brass);
	}
	FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(9.3, 0, 31), FVector(0.8, 22, 3.5), Strap);

	if (bElysia)
	{
		// Strategic's relay drone, docked flat on the upper back: a solid, flattened,
		// elongated disc. No hole, no limbs.
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(-9.6, 0, 30), FVector(1.0, 17, 7), Strap);
		FRingBuild::AddPart(Owner, Chest, ERingShape::Sphere, FVector(-12.2, 0, 30), FVector(5.5, 36, 14), FRingSurface(FLinearColor(0.02f, 0.02f, 0.022f), 0.35f, 0.6f));
		DroneLight = FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(-15.0, 0, 30), FVector(0.6, 5, 0.8), FRingSurface(FLinearColor(0.1f, 0.5f, 1.0f), 0.3f, 0.f, 6.f));
	}
	else
	{
		// Field pack and role tag.
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(-15.5, 0, 22), FVector(12, 26, 34), Slate);
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(-21.7, 0, 22), FVector(0.6, 5, 30), SlateBlue);
		FRingBuild::AddPart(Owner, Chest, ERingShape::Cube, FVector(9.8, -8, 27), FVector(0.6, 5, 7), SlateBlue);
	}

	// Head and hair.
	FRingBuild::AddPart(Owner, HeadPivot, ERingShape::Cylinder, FVector(0, 0, 3), FVector(8, 8, 7), Skin);
	FRingBuild::AddPart(Owner, HeadPivot, ERingShape::Sphere, FVector(1, 0, 15), FVector(19, 16, 22), Skin);
	if (Look == ERingBodyLook::Elysia)
	{
		FRingBuild::AddPart(Owner, HeadPivot, ERingShape::Sphere, FVector(-1.5, 0, 17.5), FVector(20, 17.5, 20), Hair);
		FRingBuild::AddPart(Owner, HeadPivot, ERingShape::Sphere, FVector(-10, 0, 22), FVector(9, 9, 9), Hair); // bun
	}
	else if (Look == ERingBodyLook::SurveyLead)
	{
		FRingBuild::AddPart(Owner, HeadPivot, ERingShape::Sphere, FVector(-1.5, 0, 18), FVector(19.5, 16.8, 19), Hair);
	}
	else
	{
		FRingBuild::AddPart(Owner, HeadPivot, ERingShape::Sphere, FVector(-1.5, 0, 15.5), FVector(21, 19, 22), Hair);
	}

	// Limbs. Placed every tick, so only size and look are set here.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Thigh[Side] = FRingBuild::AddPart(Owner, Hips, ERingShape::Cylinder, FVector::ZeroVector, FVector(14, 14, ThighLength), Suit);
		Shin[Side] = FRingBuild::AddPart(Owner, Hips, ERingShape::Cylinder, FVector::ZeroVector, FVector(11.5, 11.5, ShinLength), Suit);
		Boot[Side] = FRingBuild::AddPart(Owner, Hips, ERingShape::Cube, FVector::ZeroVector, FVector(26, 10.5, 12), Boots);
		UpperArm[Side] = FRingBuild::AddPart(Owner, Hips, ERingShape::Cylinder, FVector::ZeroVector, FVector(9, 9, UpperArmLength), Suit);
		// Elysia wears her sleeves rolled, so her forearms are bare.
		Forearm[Side] = FRingBuild::AddPart(Owner, Hips, ERingShape::Cylinder, FVector::ZeroVector, FVector(7.5, 7.5, ForearmLength), bElysia ? Skin : Suit);
		Hand[Side] = FRingBuild::AddPart(Owner, Hips, ERingShape::Sphere, FVector::ZeroVector, FVector(8, 8, 9), Boots);
	}
}

void URingBodyComponent::SetDroneLit(bool bLit)
{
	if (DroneLight)
	{
		DroneLight->SetVisibility(bLit);
	}
}

void URingBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bBuilt)
	{
		return;
	}
	Clock += DeltaTime;
	const bool bDriven = IdleStyle == ERingIdleStyle::Driven;
	const float DrivenSpeed = bDriven ? Speed : 0.f;

	MoveAlpha = FMath::FInterpTo(MoveAlpha, DrivenSpeed > 10.f ? 1.f : 0.f, DeltaTime, 8.f);
	PowerAlpha = FMath::FInterpTo(PowerAlpha, PowerTarget, DeltaTime, 6.f);
	Phase += (DrivenSpeed / StrideLength) * UE_PI * DeltaTime;
	StepCount = FMath::FloorToInt(Phase / UE_PI);

	const float Swing = MoveAlpha * FMath::Lerp(0.36f, 0.60f, FMath::Clamp((DrivenSpeed - 150.f) / 250.f, 0.f, 1.f));
	const float Sine = FMath::Sin(Phase);
	const float Cosine = FMath::Cos(Phase);

	// Breathing and a small bob.
	const float Breath = FMath::Sin(Clock * 1.3f) * 0.25f;
	float SwayY = 0.f;
	float HeadYaw = 0.f;
	if (IdleStyle == ERingIdleStyle::ConsoleWork)
	{
		HeadYaw = 10.f * FMath::Sin(Clock * 0.4f);
	}
	else if (IdleStyle == ERingIdleStyle::Inspecting)
	{
		SwayY = 1.5f * FMath::Sin(Clock * 0.5f);
		HeadYaw = 32.f * FMath::Sin(Clock * 0.23f);
	}
	Hips->SetRelativeLocation(FVector(0.0, SwayY, -MoveAlpha * 1.2f * FMath::Abs(Sine)));
	Chest->SetRelativeLocation(FVector(0.0, 0.0, 100.0 + Breath));
	HeadPivot->SetRelativeRotation(FRotator(0.f, HeadYaw, 0.f));

	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sign = SideSign(Side);
		// The right leg leads on positive sine; the left is half a cycle behind.
		const float LegAngle = Swing * Sine * Sign;
		const float Knee = MoveAlpha * 0.7f * FMath::Max(0.f, Cosine * Sign);
		const FVector Hip(0.0, HipHalfWidth * Sign, HipHeight);
		const FVector ThighDir(FMath::Sin(LegAngle), 0.0, -FMath::Cos(LegAngle));
		const FVector KneePos = Hip + ThighDir * ThighLength;
		const FVector ShinDir(FMath::Sin(LegAngle - Knee), 0.0, -FMath::Cos(LegAngle - Knee));
		const FVector Ankle = KneePos + ShinDir * ShinLength;
		FRingBuild::PlaceSegment(Thigh[Side], Hip, ThighDir, ThighLength);
		FRingBuild::PlaceSegment(Shin[Side], KneePos, ShinDir, ShinLength);
		Boot[Side]->SetRelativeLocation(Ankle + FVector(5.0, 0.0, -2.0));

		// Arms swing against the legs.
		const float ArmAngle = -0.7f * LegAngle;
		FVector UpperDir(FMath::Sin(ArmAngle), 0.10 * Sign, -FMath::Cos(ArmAngle));
		const float ForeAngle = ArmAngle + 0.20f + MoveAlpha * 0.25f;
		FVector ForeDir(FMath::Sin(ForeAngle), 0.04 * Sign, -FMath::Cos(ForeAngle));

		if (IdleStyle == ERingIdleStyle::ConsoleWork)
		{
			const float Work = 0.06f * FMath::Sin(Clock * 2.1f + Side * 1.7f);
			UpperDir = FVector(0.35, 0.05 * Sign, -0.93);
			ForeDir = FVector(0.90, -0.10 * Sign, -0.12 + Work);
		}
		else if (IdleStyle == ERingIdleStyle::Inspecting && Side == 1)
		{
			const float Work = 0.10f * FMath::Sin(Clock * 0.9f);
			UpperDir = FVector(0.20, 0.10, -0.97);
			ForeDir = FVector(0.75, -0.25, 0.30 + Work);
		}
		if (Side == 1 && PowerAlpha > 0.001f)
		{
			// The telekinesis pose: the right arm extended, hand open toward the object. Restrained, not theatrical.
			UpperDir = FMath::Lerp(UpperDir, FVector(0.80, 0.25, -0.35), PowerAlpha);
			ForeDir = FMath::Lerp(ForeDir, FVector(0.96, 0.05, 0.22), PowerAlpha);
		}
		UpperDir.Normalize();
		ForeDir.Normalize();

		const FVector Shoulder(0.0, ShoulderHalfWidth * Sign, ShoulderHeight + Breath);
		const FVector Elbow = Shoulder + UpperDir * UpperArmLength;
		const FVector Wrist = Elbow + ForeDir * ForearmLength;
		FRingBuild::PlaceSegment(UpperArm[Side], Shoulder, UpperDir, UpperArmLength);
		FRingBuild::PlaceSegment(Forearm[Side], Elbow, ForeDir, ForearmLength);
		Hand[Side]->SetRelativeLocation(Wrist + ForeDir * 3.5);
	}
}
