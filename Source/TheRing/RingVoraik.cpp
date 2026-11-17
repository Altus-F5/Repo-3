#include "RingVoraik.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "RingBuild.h"
#include "RingLogic.h"
#include "RingTuning.h"

namespace
{
	// Proportions in centimetres, in the actor's space (forward +X, feet at Z 0).
	// Index 0 is his left side (-Y), 1 his right (+Y).
	const float WaistHeight = 150.f;
	const float LongShoulderZ = 245.f;      // world height of the long-arm shoulders
	const float LongShoulderY = 30.f;
	const float LongUpperLength = 95.f;
	const float LongLowerLength = 135.f;
	const float LongTipLength = 22.f;
	const float SmallShoulderZ = 215.f;
	const float SmallShoulderY = 16.f;
	const float SmallShoulderX = 8.f;
	const float SmallUpperLength = 38.f;
	const float SmallLowerLength = 36.f;
	const float HeadTurnDegrees = 85.f;
	const float BodyTurnDegrees = 40.f;

	float SideSign(int32 Side) { return Side == 0 ? -1.f : 1.f; }

	/** Mirrors a right-side direction for the given side. */
	FVector Mirror(const FVector& RightSide, int32 Side)
	{
		return FVector(RightSide.X, RightSide.Y * SideSign(Side), RightSide.Z);
	}
}

ARingVoraik::ARingVoraik()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ARingVoraik::BeginPlay()
{
	Super::BeginPlay();
	SpeechEnd = URingTuning::Get().PrayDuration;
	BuildBody();
	StartLocation = GetActorLocation();
	ApplyPose(0.f);
}

void ARingVoraik::BuildBody()
{
	// Matte black armour, a slightly different black for cloth, satin metal for the joint collars.
	const FRingSurface Armor(FLinearColor(0.012f, 0.012f, 0.013f), 0.55f, 0.3f);
	const FRingSurface Cloth(FLinearColor(0.010f, 0.010f, 0.011f), 0.95f, 0.f);
	const FRingSurface Collar(FLinearColor(0.20f, 0.20f, 0.21f), 0.3f, 0.9f);
	const FRingSurface RedLine(FLinearColor(1.0f, 0.03f, 0.02f), 0.4f, 0.f, 12.f);

	USceneComponent* Root = RootComponent;
	Upper = FRingBuild::AddPivot(this, Root, FVector(0.0, 0.0, WaistHeight));
	Head = FRingBuild::AddPivot(this, Upper, FVector(0.0, 0.0, 272.0 - WaistHeight));

	// Legs: thin, armoured, ending in points. Mostly hidden by the cloak.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Y = 14.f * SideSign(Side);
		FRingBuild::AddPart(this, Root, ERingShape::Cylinder, FVector(0, Y, 86), FVector(11, 11, 128), Armor);
		FRingBuild::AddPart(this, Root, ERingShape::Cone, FVector(0, Y, 11), FVector(10, 10, 22), Armor, FRotator(180.f, 0.f, 0.f));
		FRingBuild::AddPart(this, Root, ERingShape::Sphere, FVector(0, Y, 82), FVector(13, 13, 13), Collar); // knee joint
	}

	// Everything from the waist up turns together.
	const float U = -WaistHeight; // converts actor heights to Upper-space heights
	FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector(0, 0, 192 + U), FVector(30, 34, 88), Armor);      // torso
	FRingBuild::AddPart(this, Upper, ERingShape::Cube, FVector(9, 0, 205 + U), FVector(14, 30, 46), Armor);           // chest plate
	FRingBuild::AddPart(this, Upper, ERingShape::Cone, FVector(0, 0, 150 + U), FVector(44, 50, 40), Armor);           // hip armour
	FRingBuild::AddPart(this, Upper, ERingShape::Cube, FVector(14, 0, 95 + U), FVector(2, 26, 150), Cloth);           // tabard, front
	FRingBuild::AddPart(this, Upper, ERingShape::Cube, FVector(-13, 0, 132 + U), FVector(3, 48, 236), Cloth);         // cloak, back panel
	FRingBuild::AddPart(this, Upper, ERingShape::Cube, FVector(-16, 0, 170 + U), FVector(3, 30, 150), Cloth);         // cloak, layered centre fold
	FRingBuild::AddPart(this, Upper, ERingShape::Cone, FVector(0, 0, 250 + U), FVector(86, 96, 34), Cloth);           // mantle
	FRingBuild::AddPart(this, Upper, ERingShape::Cone, FVector(-4, 0, 242 + U), FVector(100, 112, 30), Cloth);        // mantle, lower layer
	FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector(0, 0, 266 + U), FVector(10, 10, 16), Armor);       // neck

	// Head: narrow, flaring upward and mainly forward, shallow front to back, flatter
	// at the rear. An inverted cone squeezed in depth and pushed forward, a flat rear
	// plate, and a low cap.
	FRingBuild::AddPart(this, Head, ERingShape::Cone, FVector(5, 0, 21), FVector(22, 46, 42), Armor, FRotator(180.f, 0.f, 0.f));
	FRingBuild::AddPart(this, Head, ERingShape::Cube, FVector(-5.5, 0, 24), FVector(3, 30, 30), Armor);
	FRingBuild::AddPart(this, Head, ERingShape::Sphere, FVector(5, 0, 42), FVector(22, 46, 7), Armor);
	// The one continuous vertical red line down the face. Nothing else on the face.
	FRingBuild::AddPart(this, Head, ERingShape::Cube, FVector(11.9, 0, 24), FVector(1.0, 1.4, 40), RedLine, FRotator(-14.7f, 0.f, 0.f));

	FaceGlow = NewObject<UPointLightComponent>(this);
	FaceGlow->SetMobility(EComponentMobility::Movable);
	FaceGlow->SetupAttachment(Head);
	FaceGlow->SetRelativeLocation(FVector(20.0, 0.0, 24.0));
	FaceGlow->SetIntensityUnits(ELightUnits::Candelas);
	FaceGlow->SetIntensity(0.5f);
	FaceGlow->SetLightColor(FLinearColor(1.f, 0.05f, 0.03f));
	FaceGlow->SetAttenuationRadius(140.f);
	FaceGlow->SetCastShadows(false);
	FaceGlow->RegisterComponent();
	AddInstanceComponent(FaceGlow);

	// Arms. Placed every tick; only size and look are set here.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		LongUpper[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector::ZeroVector, FVector(9, 9, LongUpperLength), Armor);
		LongLower[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector::ZeroVector, FVector(8, 8, LongLowerLength), Armor);
		LongTip[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cone, FVector::ZeroVector, FVector(8, 8, LongTipLength), Armor);
		LongCollar[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector::ZeroVector, FVector(10.5, 10.5, 9), Collar);
		Drape[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cube, FVector::ZeroVector, FVector(3, 40, 160), Cloth);
		SmallUpper[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector::ZeroVector, FVector(5, 5, SmallUpperLength), Armor);
		SmallLower[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cylinder, FVector::ZeroVector, FVector(4, 4, SmallLowerLength), Armor);
		SmallHand[Side] = FRingBuild::AddPart(this, Upper, ERingShape::Cone, FVector::ZeroVector, FVector(5, 7, 11), Armor);
	}
}

void ARingVoraik::SetRecess(const FVector& StandLocation, AActor* InShelfObject)
{
	RecessLocation = StandLocation;
	ShelfObject = InShelfObject;
	if (ShelfObject)
	{
		ShelfObjectStart = ShelfObject->GetActorLocation();
	}
}

void ARingVoraik::StartEncounter()
{
	if (!bStarted)
	{
		bStarted = true;
		Time = 0.f;
	}
}

void ARingVoraik::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	if (bStarted)
	{
		Time += DeltaSeconds;
	}
	ApplyPose(DeltaSeconds);
}

void ARingVoraik::ApplyPose(float DeltaSeconds)
{
	if (!Upper || !Head)
	{
		return;
	}

	// --- Timeline weights ---
	const float Pray = bStarted ? 1.f - RingLogic::Ramp(Time, SpeechEnd, ArmsDownEnd) : 1.f;
	const float Walk = bStarted ? RingLogic::Ramp(Time, WalkStart, WalkEnd) : 0.f;
	const float Reach = bStarted ? RingLogic::Ramp(Time, WalkEnd, ReachEnd) * (1.f - RingLogic::Ramp(Time, TakeEnd, RetractEnd)) : 0.f;
	const float Turn = bStarted ? RingLogic::Ramp(Time, TurnStart, TurnEnd) * (1.f - RingLogic::Ramp(Time, HoldEnd, TurnBackEnd)) : 0.f;
	const bool bWalking = bStarted && Time > WalkStart && Time < WalkEnd;

	// --- Body ---
	// He crosses the floor in a slow, level glide: the cloak hides the legs.
	FVector Location = FMath::Lerp(StartLocation, bStarted ? RecessLocation : StartLocation, Walk);
	Location.Z = StartLocation.Z + (bWalking ? 0.8f * FMath::Sin(Clock * 5.f) : 0.f);
	SetActorLocation(Location);

	// The prayer has a slow sway. The aware pause is completely still.
	const float Sway = Pray * FMath::Sin(Clock * 0.7f);
	Upper->SetRelativeRotation(FRotator(-(Pray * 2.f + Sway * 1.2f), Turn * BodyTurnDegrees, 0.f));
	Head->SetRelativeRotation(FRotator(Pray * -6.f, Turn * (HeadTurnDegrees - BodyTurnDegrees), 0.f));

	// --- Long support arms ---
	// Neutral: hanging slightly outward, tips level with the pointed feet.
	// Prayer: raised and bent so the two tips meet above the head in a diamond.
	const FVector NeutralUpper(0.0, 0.26, -0.966);
	const FVector NeutralLower(0.0, 0.0, -1.0);
	const FVector PrayUpper(0.0, 0.574, 0.819);
	const FVector PrayLower(0.0, -0.538, 0.843);
	const float U = -WaistHeight;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sign = SideSign(Side);
		FVector UpperDir = FMath::Lerp(NeutralUpper, PrayUpper, Pray);
		FVector LowerDir = FMath::Lerp(NeutralLower, PrayLower, Pray);
		// Halfway through the lowering the arms pass outward, not through the body.
		const float Mid = FMath::Sin(Pray * PI);
		UpperDir.Y += 0.45 * Mid;
		LowerDir.Y += 0.25 * Mid;
		UpperDir = Mirror(UpperDir.GetSafeNormal(), Side);
		LowerDir = Mirror(LowerDir.GetSafeNormal(), Side);

		const FVector Shoulder(0.0, LongShoulderY * Sign, LongShoulderZ + U);
		const FVector Elbow = Shoulder + UpperDir * LongUpperLength;
		const FVector Wrist = Elbow + LowerDir * LongLowerLength;
		FRingBuild::PlaceSegment(LongUpper[Side], Shoulder, UpperDir, LongUpperLength);
		FRingBuild::PlaceSegment(LongLower[Side], Elbow, LowerDir, LongLowerLength);
		FRingBuild::PlaceSegment(LongTip[Side], Wrist, LowerDir, LongTipLength); // cone apex points along the arm
		FRingBuild::PlaceSegment(LongCollar[Side], Elbow - LowerDir * 4.5, LowerDir, 9.f);

		// Cloak hanging from the arm: a panel from the body out to the elbow.
		const double OuterY = FMath::Max(FMath::Abs(Elbow.Y), 34.0);
		const double InnerY = 18.0;
		const double Top = (Shoulder.Z + FMath::Max(Shoulder.Z, Elbow.Z)) * 0.5;
		const double Bottom = 28.0 + U;
		Drape[Side]->SetRelativeLocation(FVector(-9.0, (InnerY + OuterY) * 0.5 * Sign, (Top + Bottom) * 0.5));
		Drape[Side]->SetRelativeScale3D(FVector(0.03, (OuterY - InnerY) / 100.0, (Top - Bottom) / 100.0));
	}

	// --- Small manipulation arms ---
	// Neutral: folded at the chest. The right one reaches into the recess.
	const FVector FoldUpper(0.30, 0.10, -0.95);
	const FVector FoldLower(0.90, -0.20, 0.35);
	const FVector ReachUpper(0.80, 0.0, -0.10);
	const FVector ReachLower(1.0, 0.0, 0.08);
	FVector RightHandWorld = FVector::ZeroVector;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sign = SideSign(Side);
		const float SideReach = Side == 1 ? Reach : 0.f;
		const FVector UpperDir = Mirror(FMath::Lerp(FoldUpper, ReachUpper, SideReach).GetSafeNormal(), Side);
		const FVector LowerDir = Mirror(FMath::Lerp(FoldLower, ReachLower, SideReach).GetSafeNormal(), Side);
		const FVector Shoulder(SmallShoulderX, SmallShoulderY * Sign, SmallShoulderZ + U);
		const FVector Elbow = Shoulder + UpperDir * SmallUpperLength;
		const FVector Wrist = Elbow + LowerDir * SmallLowerLength;
		FRingBuild::PlaceSegment(SmallUpper[Side], Shoulder, UpperDir, SmallUpperLength);
		FRingBuild::PlaceSegment(SmallLower[Side], Elbow, LowerDir, SmallLowerLength);
		FRingBuild::PlaceSegment(SmallHand[Side], Wrist, LowerDir, 11.f);
		if (Side == 1)
		{
			RightHandWorld = Upper->GetComponentTransform().TransformPosition(Wrist + LowerDir * 12.0);
		}
	}

	// --- The object from the recess ---
	if (ShelfObject && bStarted)
	{
		if (Time >= ReachEnd && Time < TakeEnd)
		{
			// Drawn from the shelf into the hand.
			ShelfObject->SetActorLocation(FMath::Lerp(ShelfObjectStart, RightHandWorld, RingLogic::Ramp(Time, ReachEnd, TakeEnd)));
		}
		else if (Time >= TakeEnd && Time < RetractEnd)
		{
			ShelfObject->SetActorLocation(RightHandWorld);
		}
		else if (Time >= RetractEnd && !bObjectTaken)
		{
			// Carried under the cloak: what it is stays unknown.
			bObjectTaken = true;
			ShelfObject->SetActorHiddenInGame(true);
		}
	}
}
