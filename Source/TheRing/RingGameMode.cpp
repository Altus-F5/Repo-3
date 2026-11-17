#include "RingGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RingBuild.h"
#include "RingCharacter.h"
#include "RingGameInstance.h"
#include "RingHUD.h"
#include "RingLogic.h"
#include "RingPlayerController.h"
#include "RingSynthComponent.h"
#include "RingTKObject.h"
#include "RingTuning.h"
#include "RingVoraik.h"
#include "TheRing.h"

// ---------------------------------------------------------------------------
// Layout. Centimetres, Z up, floors at Z 0. The route runs along +X from the
// staging room, then turns +Y to the chamber. Everything is on one level: the
// far sector differs by having NO POWER, not by being lower.
//
//   staging room      x -600..600    y -500..500
//   partition wall    x  600..660    doorway y -150..150
//   access hall       x  660..1900   y -250..250   (floor gap x 1150..1400)
//   corridor, leg 1   x 1960..4200   y -150..150   (unpowered from here on)
//   corridor, leg 2   x 3900..4200   y  150..1650
//   chamber           centre (4050, 2250), 600 to each flat wall
// ---------------------------------------------------------------------------
namespace
{
	// Alien architecture: dark mineral-metal, smooth and monumental.
	const FRingSurface Hull(FLinearColor(0.022f, 0.024f, 0.027f), 0.42f, 0.6f);
	const FRingSurface Deck(FLinearColor(0.030f, 0.032f, 0.035f), 0.30f, 0.6f);
	const FRingSurface Rib(FLinearColor(0.040f, 0.042f, 0.046f), 0.45f, 0.6f);
	const FRingSurface GlossDeck(FLinearColor(0.020f, 0.022f, 0.025f), 0.12f, 0.7f);
	// Human expedition equipment: small, warm, practical.
	const FRingSurface CaseOlive(FLinearColor(0.16f, 0.14f, 0.09f), 0.6f, 0.1f);
	const FRingSurface CaseTrim(FLinearColor(0.30f, 0.22f, 0.10f), 0.5f, 0.6f);
	const FRingSurface TripodYellow(FLinearColor(0.60f, 0.42f, 0.05f), 0.5f, 0.3f);
	const FRingSurface DarkGear(FLinearColor(0.03f, 0.03f, 0.03f), 0.6f, 0.2f);

	const FVector StartLocation(-250.0, -150.0, 95.0);
	const float StartYaw = -20.f;
	const FVector CheckpointLocation(3000.0, 0.0, 95.0);

	const FVector BarHome(586.0, 0.0, 110.0);
	const FVector BarCleared(250.0, -330.0, 8.0);
	const FVector PartitionClosed(630.0, 0.0, 159.0);
	const float PartitionTravel = 300.f;
	const FVector PanelHome(900.0, -130.0, 8.0);
	const FVector PanelSnap(1275.0, 0.0, 6.0);

	const FBox BraceZone(FVector(541.0, -70.0, 65.0), FVector(631.0, 70.0, 155.0));
	const FBox GapZone(FVector(1130.0, -250.0, -50.0), FVector(1420.0, 250.0, 300.0));

	const FVector ChamberCentre(4050.0, 2250.0, 0.0);
	const float ChamberApothem = 600.f;
	const float ChamberHeight = 700.f;
	const FVector VoraikPrayLocation(4050.0, 2150.0, 0.0);
	const FVector VoraikRecessLocation(4050.0, 2720.0, 0.0);

	const TCHAR* SpeakerLead = TEXT("Survey Lead");
	const TCHAR* SpeakerEngineer = TEXT("Engineer");
	const TCHAR* SpeakerStrategic = TEXT("Strategic");
	const TCHAR* SpeakerElysia = TEXT("Elysia");

	/** The murmur can be heard from this far, cm. */
	const float MurmurRange = 2400.f;
}

ARingGameMode::ARingGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ARingCharacter::StaticClass();
	PlayerControllerClass = ARingPlayerController::StaticClass();
	HUDClass = ARingHUD::StaticClass();
}

void ARingGameMode::StartPlay()
{
	Super::StartPlay();

	URingGameInstance* Instance = GetGameInstance<URingGameInstance>();
	const bool bFromCheckpoint = Instance && Instance->bStartFromCheckpoint && Instance->bCheckpointReached;
	if (Instance)
	{
		Instance->bStartFromCheckpoint = false;
	}

	BuildLevel();
	PlacePlayer(bFromCheckpoint);
	bBuilt = true;
}

// ---------------------------------------------------------------------------
// Level construction
// ---------------------------------------------------------------------------

void ARingGameMode::BuildLevel()
{
	BuildStaging();
	BuildAccessHall();
	BuildCorridor();
	BuildChamber();
	BuildVista();
}

AActor* ARingGameMode::SpawnResearcher(const FVector& Location, float Yaw, ERingBodyLook Look, ERingIdleStyle Style, float Scale)
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Actor)
	{
		return nullptr;
	}
	URingBodyComponent* Body = NewObject<URingBodyComponent>(Actor);
	Body->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Body);
	Body->RegisterComponent();
	Actor->AddInstanceComponent(Body);
	Actor->SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));
	Actor->SetActorScale3D(FVector(Scale));
	Body->Build(Look);
	Body->SetIdleStyle(Style);
	// They stay put, so a simple volume keeps the player from walking through them.
	FRingBuild::PawnBlocker(World, Location + FVector(-26.0, -26.0, 0.0), Location + FVector(26.0, 26.0, 185.0));
	return Actor;
}

void ARingGameMode::BuildStaging()
{
	UWorld* W = GetWorld();
	const URingTuning& Tuning = URingTuning::Get();

	// Shell.
	FRingBuild::Slab(W, FVector(-660, -560, -40), FVector(660, 560, 0), Deck);
	FRingBuild::Slab(W, FVector(-660, -560, 450), FVector(660, 560, 490), Hull);
	FRingBuild::Slab(W, FVector(-660, -560, 0), FVector(-600, 560, 450), Hull);   // west
	FRingBuild::Slab(W, FVector(-600, 500, 0), FVector(600, 560, 450), Hull);     // north

	// South wall with the observation gap: a long opening onto the ring.
	FRingBuild::Slab(W, FVector(-600, -560, 0), FVector(600, -500, 105), Hull);
	FRingBuild::Slab(W, FVector(-600, -560, 335), FVector(600, -500, 450), Hull);
	FRingBuild::Slab(W, FVector(-600, -560, 105), FVector(-380, -500, 335), Hull);
	FRingBuild::Slab(W, FVector(380, -560, 105), FVector(600, -500, 335), Hull);
	FRingBuild::Slab(W, FVector(-130, -545, 105), FVector(-118, -515, 335), Rib);
	FRingBuild::Slab(W, FVector(118, -545, 105), FVector(130, -515, 335), Rib);

	// East wall: solid to the south, a pocket to the north that the partition slides into.
	FRingBuild::Slab(W, FVector(600, -560, 0), FVector(660, -150, 450), Hull);
	FRingBuild::Slab(W, FVector(600, -150, 320), FVector(660, 150, 450), Hull);
	FRingBuild::Slab(W, FVector(600, 150, 0), FVector(616, 560, 450), Hull);
	FRingBuild::Slab(W, FVector(644, 150, 0), FVector(660, 560, 450), Hull);
	FRingBuild::Slab(W, FVector(616, 470, 0), FVector(644, 560, 450), Hull);

	// Monumental ribs along the walls.
	for (int32 X = -450; X <= 450; X += 300)
	{
		FRingBuild::Slab(W, FVector(X - 20, 480, 0), FVector(X + 20, 500, 450), Rib);
	}
	for (int32 Y = -300; Y <= 300; Y += 300)
	{
		FRingBuild::Slab(W, FVector(-600, Y - 20, 0), FVector(-580, Y + 20, 450), Rib);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The partition: heavy, on a track, moved by telekinesis only. Constrained so it cannot leave the wall.
	Partition = W->SpawnActor<ARingTKObject>(ARingTKObject::StaticClass(), FTransform(PartitionClosed), Params);
	Partition->Configure(TEXT("Partition"), FVector(20, 296, 314), FRingSurface(FLinearColor(0.035f, 0.036f, 0.040f), 0.5f, 0.6f));
	Partition->ConfigureTrack(FVector(0, 1, 0), PartitionTravel);
	Partition->JammedReason = TEXT("Braced shut. Clear the bar first.");

	// The brace bar across it, resting in two brackets. The first thing the player moves.
	FRingBuild::Slab(W, FVector(574, 160, 92), FVector(600, 190, 104), Rib);
	FRingBuild::Slab(W, FVector(574, -190, 92), FVector(600, -160, 104), Rib);
	Bar = W->SpawnActor<ARingTKObject>(ARingTKObject::StaticClass(), FTransform(BarHome), Params);
	Bar->Configure(TEXT("Brace bar"), FVector(12, 380, 12), FRingSurface(FLinearColor(0.07f, 0.07f, 0.075f), 0.5f, 0.7f));

	// --- Human equipment ---
	// Work light on a stand: the one warm light source.
	const FVector LampHead(-250, 380, 200);
	FRingBuild::Block(W, FVector(-250, 392, 95), FVector(5, 5, 190), TripodYellow, FRotator::ZeroRotator, false, true, ERingShape::Cylinder);
	FRingBuild::Block(W, FVector(-250, 392, 6), FVector(70, 70, 4), TripodYellow, FRotator::ZeroRotator, false, true, ERingShape::Cylinder);
	FRingBuild::Block(W, LampHead + FVector(0, 6, 0), FVector(30, 10, 24), DarkGear, FRotator::ZeroRotator, false);
	FRingBuild::Block(W, LampHead + FVector(0, 0, 0), FVector(26, 2, 20), FRingSurface(FLinearColor(1.0f, 0.78f, 0.45f), 0.4f, 0.f, 20.f), FRotator::ZeroRotator, false, false);
	FRingBuild::SpotLight(W, LampHead + FVector(0, -8, 0), (FVector(150, -100, 40) - LampHead).Rotation(), Tuning.WorkLightCandela,
		FLinearColor(1.0f, 0.80f, 0.55f), 1700.f, 38.f, 62.f, true);

	// Cases.
	FRingBuild::Slab(W, FVector(-475, 365, 0), FVector(-365, 435, 50), CaseOlive);
	FRingBuild::Slab(W, FVector(-465, 370, 50), FVector(-375, 430, 90), CaseOlive);
	FRingBuild::Slab(W, FVector(-477, 398, 20), FVector(-363, 402, 28), CaseTrim, false);
	FRingBuild::Slab(W, FVector(-555, 205, 0), FVector(-485, 295, 44), CaseOlive);

	// Folding table with the survey slate.
	FRingBuild::Slab(W, FVector(35, 400, 74), FVector(165, 460, 80), CaseOlive);
	FRingBuild::Slab(W, FVector(40, 405, 0), FVector(46, 411, 74), DarkGear, false);
	FRingBuild::Slab(W, FVector(154, 405, 0), FVector(160, 411, 74), DarkGear, false);
	FRingBuild::Slab(W, FVector(40, 449, 0), FVector(46, 455, 74), DarkGear, false);
	FRingBuild::Slab(W, FVector(154, 449, 0), FVector(160, 455, 74), DarkGear, false);
	FRingBuild::Block(W, FVector(100, 446, 104), FVector(62, 5, 42), DarkGear, FRotator(0.f, 0.f, -12.f), false);
	FRingBuild::Block(W, FVector(100, 443, 104), FVector(56, 1, 36), FRingSurface(FLinearColor(0.15f, 0.55f, 0.50f), 0.3f, 0.f, 4.f), FRotator(0.f, 0.f, -12.f), false, false);
	FRingBuild::PointLight(W, FVector(100, 425, 108), 1.5f, FLinearColor(0.35f, 0.9f, 0.8f), 220.f, false);

	// The two researchers. They stay with the equipment.
	SurveyLead = SpawnResearcher(FVector(100, 365, 0), 90.f, ERingBodyLook::SurveyLead, ERingIdleStyle::ConsoleWork, 1.05f);
	Engineer = SpawnResearcher(FVector(-330, 300, 0), 150.f, ERingBodyLook::Engineer, ERingIdleStyle::Inspecting, 0.97f);

	InteractPoints.Add({TEXT("lead"), FVector(100, 365, 100), TEXT("Talk to the survey lead")});
	InteractPoints.Add({TEXT("engineer"), FVector(-330, 300, 100), TEXT("Talk to the engineer")});
	InteractPoints.Add({TEXT("gap"), FVector(0, -470, 110), TEXT("Look out over the ring")});
}

void ARingGameMode::BuildAccessHall()
{
	UWorld* W = GetWorld();

	// Floor on both sides of a collapsed section, and the drop between.
	FRingBuild::Slab(W, FVector(660, -310, -40), FVector(1150, 310, 0), Deck);
	FRingBuild::Slab(W, FVector(1400, -310, -40), FVector(1960, 310, 0), Deck);
	FRingBuild::Slab(W, FVector(1110, -310, -400), FVector(1150, 310, -40), Hull);
	FRingBuild::Slab(W, FVector(1400, -310, -400), FVector(1440, 310, -40), Hull);
	FRingBuild::Slab(W, FVector(1110, -310, -440), FVector(1440, 310, -400), Hull);
	FRingBuild::Slab(W, FVector(660, 250, -400), FVector(1960, 310, 380), Hull);
	FRingBuild::Slab(W, FVector(660, -310, -400), FVector(1960, -250, 380), Hull);
	FRingBuild::Slab(W, FVector(660, -310, 380), FVector(1960, 310, 420), Hull);

	// East end: the doorway into the unpowered sector.
	FRingBuild::Slab(W, FVector(1900, -310, 0), FVector(1960, -110, 380), Hull);
	FRingBuild::Slab(W, FVector(1900, 110, 0), FVector(1960, 310, 380), Hull);
	FRingBuild::Slab(W, FVector(1900, -110, 300), FVector(1960, 110, 380), Hull);

	// Broken edges and fallen fragments. Dressing only: none of it blocks or can be moved.
	FRandomStream Random(1107);
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const bool bNear = Index % 2 == 0;
		const FVector Position(bNear ? Random.FRandRange(1085.f, 1150.f) : Random.FRandRange(1400.f, 1465.f), Random.FRandRange(-235.f, 235.f), Random.FRandRange(2.f, 9.f));
		if (FMath::Abs(Position.Y) < 80.0)
		{
			continue; // keep the crossing clear
		}
		const FVector Size(Random.FRandRange(14.f, 44.f), Random.FRandRange(12.f, 36.f), Random.FRandRange(6.f, 18.f));
		FRingBuild::Block(W, Position, Size, Rib, FRotator(Random.FRandRange(-12.f, 12.f), Random.FRandRange(0.f, 180.f), Random.FRandRange(-12.f, 12.f)), false);
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FVector Position(Random.FRandRange(1170.f, 1380.f), Random.FRandRange(-220.f, 220.f), -385.0);
		FRingBuild::Block(W, Position, FVector(Random.FRandRange(40.f, 110.f), Random.FRandRange(30.f, 80.f), 30.0), Rib,
			FRotator(Random.FRandRange(-15.f, 15.f), Random.FRandRange(0.f, 180.f), 0.f), false);
	}

	// Two low fixtures that still work. This side of the far doorway has power.
	FRingBuild::Slab(W, FVector(1040, -250, 14), FVector(1090, -246, 24), FRingSurface(FLinearColor(0.75f, 0.95f, 1.0f), 0.3f, 0.f, 10.f), false);
	FRingBuild::PointLight(W, FVector(1065, -225, 25), 9.f, FLinearColor(0.75f, 0.93f, 1.0f), 700.f, false);
	FRingBuild::Slab(W, FVector(1470, 246, 14), FVector(1520, 250, 24), FRingSurface(FLinearColor(0.75f, 0.95f, 1.0f), 0.3f, 0.f, 10.f), false);
	FRingBuild::PointLight(W, FVector(1495, 225, 25), 9.f, FLinearColor(0.75f, 0.93f, 1.0f), 700.f, false);

	// The player can only cross on the seated panel. No jumping, no edge to fall from.
	FRingBuild::PawnBlocker(W, FVector(1150, -250, 0), FVector(1400, -65, 250));
	FRingBuild::PawnBlocker(W, FVector(1150, 65, 0), FVector(1400, 250, 250));
	GapBlocker = FRingBuild::PawnBlocker(W, FVector(1150, -65, 0), FVector(1400, 65, 250));

	// The support position: ledges at both edges and a thin frame outlining where the panel sits.
	const FRingSurface Marker(FLinearColor(0.42f, 0.29f, 0.10f), 0.4f, 0.85f);
	FRingBuild::Slab(W, FVector(1110, -66, 0.5), FVector(1440, -63, 2.5), Marker, false);
	FRingBuild::Slab(W, FVector(1110, 63, 0.5), FVector(1440, 66, 2.5), Marker, false);
	FRingBuild::Slab(W, FVector(1110, -66, 0.5), FVector(1113, 66, 2.5), Marker, false);
	FRingBuild::Slab(W, FVector(1437, -66, 0.5), FVector(1440, 66, 2.5), Marker, false);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Panel = W->SpawnActor<ARingTKObject>(ARingTKObject::StaticClass(), FTransform(PanelHome), Params);
	Panel->Configure(TEXT("Deck panel"), FVector(330, 130, 12), FRingSurface(FLinearColor(0.06f, 0.06f, 0.065f), 0.5f, 0.7f));
	Panel->ConfigureSnap(PanelSnap, URingTuning::Get().SnapRadius);
}

void ARingGameMode::BuildCorridor()
{
	UWorld* W = GetWorld();

	// Leg 1, running +X. No fixtures, no screens, no glow: this sector has no power.
	FRingBuild::Slab(W, FVector(1960, -210, -40), FVector(4260, 210, 0), Deck);
	FRingBuild::Slab(W, FVector(1960, -210, 340), FVector(4260, 210, 380), Hull);
	FRingBuild::Slab(W, FVector(1960, -210, 0), FVector(4260, -150, 340), Hull);
	FRingBuild::Slab(W, FVector(1960, 150, 0), FVector(3900, 210, 340), Hull);
	FRingBuild::Slab(W, FVector(4200, -210, 0), FVector(4260, 150, 340), Hull);
	for (int32 X = 2140; X <= 3780; X += 280)
	{
		FRingBuild::Slab(W, FVector(X - 14, -150, 0), FVector(X + 14, -136, 340), Rib, false);
		FRingBuild::Slab(W, FVector(X - 14, 136, 0), FVector(X + 14, 150, 340), Rib, false);
		FRingBuild::Slab(W, FVector(X - 14, -150, 322), FVector(X + 14, 150, 340), Rib, false);
	}

	// Leg 2, running +Y to the chamber, with the observation alcove on its east side.
	FRingBuild::Slab(W, FVector(3840, 210, -40), FVector(4480, 1650, 0), Deck);
	FRingBuild::Slab(W, FVector(3840, 210, 340), FVector(4480, 1650, 380), Hull);
	FRingBuild::Slab(W, FVector(3840, 210, 0), FVector(3900, 1650, 340), Hull);
	FRingBuild::Slab(W, FVector(4200, 150, 0), FVector(4260, 1280, 340), Hull);
	FRingBuild::Slab(W, FVector(4200, 1220, 0), FVector(4400, 1280, 340), Hull);
	FRingBuild::Slab(W, FVector(4400, 1220, 0), FVector(4460, 1540, 340), Hull);
	FRingBuild::Slab(W, FVector(4200, 1480, 0), FVector(4400, 1540, 340), Hull);
	FRingBuild::Slab(W, FVector(4200, 1540, 0), FVector(4260, 1650, 340), Hull);
	for (int32 Y = 400; Y <= 1100; Y += 280)
	{
		FRingBuild::Slab(W, FVector(3900, Y - 14, 0), FVector(3914, Y + 14, 340), Rib, false);
		FRingBuild::Slab(W, FVector(4186, Y - 14, 0), FVector(4200, Y + 14, 340), Rib, false);
		FRingBuild::Slab(W, FVector(3900, Y - 14, 322), FVector(4200, Y + 14, 340), Rib, false);
	}

	// The direct way into the chamber is choked with collapsed structure: low enough
	// to see over, impossible to walk through. It is the credible limit on the encounter.
	FRandomStream Random(2203);
	for (int32 Index = 0; Index < 16; ++Index)
	{
		const FVector Size(Random.FRandRange(50.f, 130.f), Random.FRandRange(30.f, 70.f), Random.FRandRange(20.f, 46.f));
		const FVector Position(Random.FRandRange(3930.f, 4170.f), Random.FRandRange(1565.f, 1640.f), Random.FRandRange(12.f, 78.f));
		FRingBuild::Block(W, Position, Size, Rib, FRotator(Random.FRandRange(-18.f, 18.f), Random.FRandRange(0.f, 180.f), Random.FRandRange(-18.f, 18.f)), false);
	}
	FRingBuild::Block(W, FVector(3975, 1600, 62), FVector(300, 28, 22), Hull, FRotator(14.f, 12.f, 0.f), false);
	FRingBuild::Block(W, FVector(4120, 1610, 48), FVector(240, 24, 20), Hull, FRotator(-10.f, -20.f, 0.f), false);
	FRingBuild::PawnBlocker(W, FVector(3900, 1550, 0), FVector(4200, 1650, 340));
}

void ARingGameMode::BuildChamber()
{
	UWorld* W = GetWorld();
	const URingTuning& Tuning = URingTuning::Get();

	FRingBuild::Slab(W, FVector(3350, 1650, -40), FVector(4750, 2950, 0), GlossDeck);
	FRingBuild::Slab(W, FVector(3350, 1650, ChamberHeight), FVector(4750, 2950, ChamberHeight + 40), Hull);

	// Sixteen flat wall sections. The one facing the corridor is left open.
	const int32 Sides = 16;
	const int32 DoorSide = 12; // 270 degrees: the -Y side
	for (int32 Index = 0; Index < Sides; ++Index)
	{
		if (Index == DoorSide)
		{
			continue;
		}
		const float Degrees = Index * 360.f / Sides;
		const FVector Normal(FMath::Cos(FMath::DegreesToRadians(Degrees)), FMath::Sin(FMath::DegreesToRadians(Degrees)), 0.0);
		const FVector Centre = ChamberCentre + Normal * (ChamberApothem + 30.f) + FVector(0.0, 0.0, ChamberHeight * 0.5);
		FRingBuild::Block(W, Centre, FVector(60, 252, ChamberHeight), Hull, FRotator(0.f, Degrees, 0.f));
	}
	FRingBuild::Slab(W, FVector(3900, 1650, 340), FVector(4200, 1712, ChamberHeight), Hull); // above the doorway

	// The recess in the far wall, and the small unidentified object on its shelf.
	FRingBuild::Slab(W, FVector(3990, 2778, 190), FVector(4090, 2850, 198), Rib, false);
	FRingBuild::Slab(W, FVector(3982, 2778, 190), FVector(3990, 2850, 262), Rib, false);
	FRingBuild::Slab(W, FVector(4090, 2778, 190), FVector(4098, 2850, 262), Rib, false);
	FRingBuild::Slab(W, FVector(3982, 2778, 262), FVector(4098, 2850, 270), Rib, false);
	AStaticMeshActor* ShelfObject = FRingBuild::Block(W, FVector(4034, 2798, 203), FVector(12, 22, 7), FRingSurface(FLinearColor(0.03f, 0.03f, 0.032f), 0.35f, 0.8f),
		FRotator::ZeroRotator, false, false);

	// Light. The room has no power. What shows him is a cold spill through a breach
	// high in the far wall: it rims his head, shoulders and raised arms from beyond,
	// and streaks the floor toward the doorway. Nothing in the room is switched on.
	FRingBuild::Block(W, FVector(3930, 2810, 640), FVector(220, 40, 26), Hull, FRotator(0.f, 8.f, 22.f), false);
	FRingBuild::Block(W, FVector(4170, 2800, 655), FVector(200, 40, 26), Hull, FRotator(0.f, -10.f, -18.f), false);
	const FVector RimSource(4050, 2815, 660);
	FRingBuild::SpotLight(W, RimSource, (FVector(4050, 2300, 110) - RimSource).Rotation(), Tuning.ChamberRimCandela,
		FLinearColor(0.72f, 0.86f, 0.90f), 2200.f, 16.f, 30.f, true);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Facing +Y: his back is to the doorway.
	Voraik = W->SpawnActor<ARingVoraik>(ARingVoraik::StaticClass(), FTransform(FRotator(0.f, 90.f, 0.f), VoraikPrayLocation), Params);
	if (Voraik)
	{
		Voraik->SetRecess(VoraikRecessLocation, ShelfObject);
	}
}

void ARingGameMode::BuildVista()
{
	UWorld* W = GetWorld();
	const URingTuning& Tuning = URingTuning::Get();

	// The ring itself, seen through the observation gap: a band tens of kilometres
	// wide curving up and away in both directions. Distant geometry only: no
	// collision, no shadows, nothing the player can reach.
	const double Radius = 60000.0;
	const double BandCentreY = -9500.0;
	const FRingSurface RingHull(FLinearColor(0.045f, 0.047f, 0.050f), 0.6f, 0.4f);
	const FRingSurface Garden(FLinearColor(0.035f, 0.075f, 0.040f), 0.9f, 0.f);
	const FRingSurface Water(FLinearColor(0.03f, 0.10f, 0.12f), 0.15f, 0.2f);
	const FRingSurface Ruin(FLinearColor(0.09f, 0.09f, 0.085f), 0.8f, 0.1f);
	FRandomStream Random(4242);
	const double StepDegrees = 4.0;
	const double SegmentLength = Radius * FMath::DegreesToRadians(StepDegrees) * 1.04;
	for (double Degrees = -64.0; Degrees <= 64.0; Degrees += StepDegrees)
	{
		const double Radians = FMath::DegreesToRadians(Degrees);
		const FVector Centre(Radius * FMath::Sin(Radians), BandCentreY, Radius * (1.0 - FMath::Cos(Radians)) - 900.0);
		const FRotator Tilt(static_cast<float>(Degrees), 0.f, 0.f);
		FRingBuild::Block(W, Centre, FVector(SegmentLength, 15000, 500), RingHull, Tilt, false, false);
		// The far rim wall.
		FRingBuild::Block(W, Centre + Tilt.RotateVector(FVector(0, -7600, 1700)), FVector(SegmentLength, 500, 3400), RingHull, Tilt, false, false);
		// Overgrown terraces, water, and what is left of the builders' structures.
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FVector Local(Random.FRandRange(-1800.f, 1800.f), Random.FRandRange(-6500.f, 6200.f), 0.0);
			const int32 Kind = Random.RandRange(0, 3);
			if (Kind == 0)
			{
				FRingBuild::Block(W, Centre + Tilt.RotateVector(Local + FVector(0, 0, 256)), FVector(Random.FRandRange(900.f, 2600.f), Random.FRandRange(900.f, 2600.f), 12), Garden, Tilt, false, false);
			}
			else if (Kind == 1)
			{
				FRingBuild::Block(W, Centre + Tilt.RotateVector(Local + FVector(0, 0, 254)), FVector(Random.FRandRange(600.f, 1800.f), Random.FRandRange(2000.f, 5000.f), 8), Water, Tilt, false, false);
			}
			else
			{
				const float Height = Random.FRandRange(300.f, 1400.f);
				FRingBuild::Block(W, Centre + Tilt.RotateVector(Local + FVector(0, 0, 250 + Height * 0.5f)), FVector(Random.FRandRange(120.f, 420.f), Random.FRandRange(120.f, 900.f), Height), Ruin, Tilt, false, false);
			}
		}
	}

	// The wall the staging room is set into, continuing either side of the gap.
	FRingBuild::Block(W, FVector(-9000, -640, 1500), FVector(16600, 160, 9000), RingHull, FRotator::ZeroRotator, false, false);
	FRingBuild::Block(W, FVector(9000, -640, 1500), FVector(16600, 160, 9000), RingHull, FRotator::ZeroRotator, false, false);

	// The system's star, low over the band. It lights the vista and throws one hard
	// shaft through the gap into the staging room.
	FRingBuild::StarLight(W, FVector(0.2, 0.9, -0.35).Rotation(), Tuning.StarLux, FLinearColor(0.85f, 0.92f, 1.0f));

	// Stars, only when an emissive material is available to draw them with.
	if (FRingBuild::HasEmissive())
	{
		const FRingSurface StarSurface(FLinearColor(0.9f, 0.95f, 1.0f), 0.5f, 0.f, 8.f);
		for (int32 Index = 0; Index < 140; ++Index)
		{
			FVector Direction(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, -0.05f), Random.FRandRange(-0.5f, 1.f));
			Direction.Normalize();
			const float Size = Random.FRandRange(60.f, 150.f);
			FRingBuild::Block(W, Direction * 90000.0, FVector(Size), StarSurface, FRotator::ZeroRotator, false, false, ERingShape::Sphere);
		}
	}
}

// ---------------------------------------------------------------------------
// Start, checkpoint, restart, reset
// ---------------------------------------------------------------------------

void ARingGameMode::PlacePlayer(bool bFromCheckpoint)
{
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	Player = Controller ? Cast<ARingCharacter>(Controller->GetPawn()) : nullptr;
	if (!Player)
	{
		UE_LOG(LogTheRing, Error, TEXT("No ARingCharacter was spawned for the player; the demo cannot start."));
		return;
	}

	const FVector Location = bFromCheckpoint ? CheckpointLocation : StartLocation;
	const FRotator Rotation(0.f, bFromCheckpoint ? 0.f : StartYaw, 0.f);
	Player->GetCharacterMovement()->StopMovementImmediately();
	Player->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	Controller->SetControlRotation(Rotation);
	LastSafeLocation = Location;
	if (ARingPlayerController* RingController = Cast<ARingPlayerController>(Controller))
	{
		RingController->ApplySettings();
	}

	Fade = 1.f;
	FadeTarget = 0.f;
	FiredBeats.Reset();
	Lines.Reset();
	StageTime = 0.f;

	if (bFromCheckpoint)
	{
		// The route behind the checkpoint is restored as solved.
		Bar->PlaceResting(BarCleared);
		Partition->PlaceOnTrack(1.f);
		Panel->PlaceSnapped();
		Stage = ERingStage::Approach;
		ShowToast(TEXT("Restarted from checkpoint"));
	}
	else
	{
		Stage = ERingStage::Arrival;
	}
	UpdateObjective();
}

void ARingGameMode::Reload()
{
	UGameplayStatics::SetGamePaused(this, false);
	const FString Map = UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName());
	UGameplayStatics::OpenLevel(this, FName(*Map));
}

void ARingGameMode::RestartFromCheckpoint()
{
	if (URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		Instance->bStartFromCheckpoint = Instance->bCheckpointReached;
	}
	Reload();
}

void ARingGameMode::RestartDemo()
{
	if (URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		Instance->ClearCheckpoint();
	}
	Reload();
}

void ARingGameMode::ResetPuzzleObjects()
{
	if (!Player || !bBuilt)
	{
		return;
	}
	Player->ForceRelease();
	FVector Location = Player->GetActorLocation();

	// On the staging side everything goes back to how it was found, so the lesson
	// can be repeated. Past the partition, the bar and partition are left solved:
	// re-bracing a door the player is behind would trap them.
	if (Location.X < 600.0)
	{
		Bar->ResetToHome();
		Partition->PlaceOnTrack(0.f);
	}
	else
	{
		Bar->PlaceResting(BarCleared);
		Partition->PlaceOnTrack(1.f);
	}

	// The panel always returns to where it was found. It is within reach from both
	// sides of the gap. Anyone standing on it is moved to the nearer edge first.
	if (GapZone.IsInsideOrOn(Location))
	{
		Location.X = Location.X < PanelSnap.X ? 1085.0 : 1465.0;
		Location.Y = 0.0;
		Player->GetCharacterMovement()->StopMovementImmediately();
		Player->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	}
	Panel->ResetToHome();
	ShowToast(TEXT("Puzzle objects reset"));
}

// ---------------------------------------------------------------------------
// Progression
// ---------------------------------------------------------------------------

void ARingGameMode::Advance(ERingStage To)
{
	if (!RingLogic::CanAdvance(Stage, To))
	{
		return;
	}
	Stage = To;
	StageTime = 0.f;
	UpdateObjective();
}

void ARingGameMode::UpdateObjective()
{
	switch (Stage)
	{
	case ERingStage::Arrival: Objective = TEXT("Join the team at the staging area"); break;
	case ERingStage::ClearBrace: Objective = TEXT("Clear the brace bar from the partition"); break;
	case ERingStage::OpenPartition: Objective = TEXT("Slide the partition open"); break;
	case ERingStage::PlaceBridge: Objective = TEXT("Bridge the broken floor with the deck panel"); break;
	case ERingStage::EnterCorridor: Objective = TEXT("Go on into the adjoining sector"); break;
	case ERingStage::Approach: Objective = TEXT("Find the source of the movement"); break;
	case ERingStage::Encounter: Objective = TEXT("Stay out of sight. Watch."); break;
	case ERingStage::Retreat: Objective = TEXT("Return to the team. Quietly."); break;
	default: Objective.Reset(); break;
	}
}

bool ARingGameMode::Once(FName Beat)
{
	if (FiredBeats.Contains(Beat))
	{
		return false;
	}
	FiredBeats.Add(Beat);
	return true;
}

void ARingGameMode::Say(const FString& Speaker, const FString& Text, float Duration)
{
	FRingLine Line;
	Line.Speaker = Speaker;
	Line.Text = Text;
	Line.Duration = Duration > 0.f ? Duration : RingLogic::SubtitleDuration(Text.Len());
	if (Lines.Num() == 0)
	{
		LineTime = 0.f;
	}
	Lines.Add(Line);
}

void ARingGameMode::TickDialogue(float DeltaSeconds)
{
	// One line at a time, in order. Lines queue; they never overlap.
	if (Lines.Num() == 0)
	{
		return;
	}
	LineTime += DeltaSeconds;
	if (LineTime >= Lines[0].Duration + 0.35f)
	{
		Lines.RemoveAt(0);
		LineTime = 0.f;
	}
}

bool ARingGameMode::GetSubtitle(FString& OutSpeaker, FString& OutText) const
{
	if (Lines.Num() == 0 || LineTime > Lines[0].Duration)
	{
		return false;
	}
	OutSpeaker = Lines[0].Speaker;
	OutText = Lines[0].Text;
	return true;
}

bool ARingGameMode::GetSnapMarker(FVector& OutLocation) const
{
	if (!Panel || Panel->IsSnapped() || !Panel->IsHeld())
	{
		return false;
	}
	OutLocation = Panel->GetSnapLocation();
	return true;
}

void ARingGameMode::ShowToast(const FString& Text)
{
	Toast = Text;
	ToastTime = 3.f;
}

void ARingGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBuilt || !Player || !Bar || !Partition || !Panel)
	{
		return;
	}
	StageTime += DeltaSeconds;
	TickDialogue(DeltaSeconds);
	Fade = FMath::FInterpConstantTo(Fade, FadeTarget, DeltaSeconds, 0.7f);
	if (ToastTime > 0.f)
	{
		ToastTime -= DeltaSeconds;
		if (ToastTime <= 0.f)
		{
			Toast.Reset();
		}
	}

	const FVector Location = Player->GetActorLocation();

	// Puzzle bookkeeping.
	Partition->bJammed = BraceZone.IsInsideOrOn(Bar->GetActorLocation());
	const bool bBridged = Panel->IsSnapped();
	if (bBridged != bBridgeOpen && GapBlocker)
	{
		bBridgeOpen = bBridged;
		GapBlocker->GetStaticMeshComponent()->SetCollisionEnabled(bBridged ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
	const bool bInGap = GapZone.IsInsideOrOn(Location);
	Panel->bPlayerStandingOn = bBridged && bInGap;

	// Recovery: if she is ever somewhere she should not be, put her back on solid floor.
	if (Location.Z < -150.0)
	{
		Player->GetCharacterMovement()->StopMovementImmediately();
		Player->SetActorLocation(LastSafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else if (!bInGap && Player->GetCharacterMovement()->IsMovingOnGround())
	{
		LastSafeLocation = Location;
	}

	TickStage(Location, DeltaSeconds);
	TickAtmosphere(Location, DeltaSeconds);
	UpdatePrompt(Location);
}

void ARingGameMode::TickStage(const FVector& L, float DeltaSeconds)
{
	const bool bLegTwo = L.X > 3880.0 && L.Y > 160.0;

	switch (Stage)
	{
	case ERingStage::Arrival:
		if (Once(TEXT("intro")))
		{
			Say(SpeakerLead, TEXT("Lights are holding. We'll get the equipment secured."));
			Say(SpeakerStrategic, TEXT("Relay is stable. The station is still out of the system, so I'm caching locally until it returns."));
			Say(SpeakerEngineer, TEXT("The partition into the next section is braced from this side. Elysia, can you clear it?"));
		}
		if ((StageTime > 3.f && !IsTalking()) || L.X > 250.0)
		{
			Advance(ERingStage::ClearBrace);
		}
		break;

	case ERingStage::ClearBrace:
		if (!Partition->bJammed && !Bar->IsHeld())
		{
			Say(SpeakerStrategic, TEXT("Brace is clear. The partition should travel on its track now."));
			Advance(ERingStage::OpenPartition);
		}
		break;

	case ERingStage::OpenPartition:
		if (Partition->GetTrackAlpha() >= 0.85f)
		{
			Say(SpeakerEngineer, TEXT("That's open. Go ahead, we'll stay with the cases."));
			Advance(ERingStage::PlaceBridge);
		}
		break;

	case ERingStage::PlaceBridge:
		if (L.X > 800.0 && Once(TEXT("gap")))
		{
			Say(SpeakerStrategic, TEXT("The floor section ahead is gone. That loose deck panel would span it."));
		}
		if (Panel->IsSnapped() && !Panel->IsHeld())
		{
			Say(SpeakerLead, TEXT("We'll hold here and secure the equipment. Don't go far."));
			Advance(ERingStage::EnterCorridor);
		}
		break;

	case ERingStage::EnterCorridor:
		if (L.X > 1960.0 && Once(TEXT("dark")))
		{
			Say(SpeakerStrategic, TEXT("No power in this sector. Nothing on any circuit."));
		}
		if (L.X > 2500.0 && Once(TEXT("discrepancy")))
		{
			Say(SpeakerStrategic, TEXT("One moment. I have a discrepancy. A mass reading near the ring that was not there on the last pass."));
			Say(SpeakerStrategic, TEXT("I can't resolve it. Nothing on visual. Logging it."));
		}
		if (L.X > CheckpointLocation.X)
		{
			if (URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
			{
				Instance->MarkCheckpoint();
			}
			ShowToast(TEXT("Checkpoint saved"));
			Advance(ERingStage::Approach);
		}
		break;

	case ERingStage::Approach:
		if (bLegTwo && L.Y > 420.0 && Once(TEXT("passive")))
		{
			Say(SpeakerStrategic, TEXT("Movement ahead, and it is not ours. Going passive. Light off."));
		}
		if (bLegTwo && L.Y > 1050.0)
		{
			FiredBeats.Add(TEXT("passive"));
			if (Voraik)
			{
				Voraik->StartEncounter();
			}
			Lines.Reset(); // Strategic is silent from here
			Say(FString(), TEXT("Quiet speech in an unfamiliar language"), 6.f);
			Advance(ERingStage::Encounter);
		}
		break;

	case ERingStage::Encounter:
	{
		if (Voraik)
		{
			if (Voraik->IsSpeaking() && Voraik->GetEncounterTime() > 9.f && Once(TEXT("speech2")))
			{
				Say(FString(), TEXT("The quiet speech continues"), 5.f);
			}
			if (!Voraik->IsSpeaking() && Once(TEXT("speechstop")))
			{
				Say(FString(), TEXT("The speech stops"), 3.5f);
			}
			if (Voraik->IsInAwarePause() && Once(TEXT("still")))
			{
				Say(FString(), TEXT("Silence"), 3.f);
			}
		}
		// The outcome is fixed: she sees someone, and is not seen. If the player
		// backs out early the encounter still counts as witnessed.
		const bool bWithdrew = !bLegTwo || L.Y < 600.0;
		if (!Voraik || Voraik->IsEncounterComplete() || (bWithdrew && StageTime > 3.f))
		{
			Advance(ERingStage::Retreat);
		}
		break;
	}

	case ERingStage::Retreat:
		if (L.X < 540.0)
		{
			Advance(ERingStage::Ending);
		}
		break;

	case ERingStage::Ending:
		if (Once(TEXT("ending")))
		{
			for (AActor* Researcher : {SurveyLead.Get(), Engineer.Get()})
			{
				if (Researcher)
				{
					const FRotator Facing = (L - Researcher->GetActorLocation()).Rotation();
					Researcher->SetActorRotation(FRotator(0.f, Facing.Yaw, 0.f));
				}
			}
			Lines.Reset();
			Say(SpeakerElysia, TEXT("Someone is here."));
			Say(SpeakerLead, TEXT("Another research team is here?"));
			Say(SpeakerElysia, TEXT("Not human."));
			Say(SpeakerLead, TEXT("Did it see you?"));
			Say(SpeakerElysia, TEXT("I don't think so."), 3.5f);
			EndingQuiet = 0.f;
		}
		else if (!IsTalking())
		{
			// Nobody answers. Hold on that, then close.
			EndingQuiet += DeltaSeconds;
			if (EndingQuiet > 3.f)
			{
				FadeTarget = 1.f;
			}
			if (Fade >= 0.99f)
			{
				Advance(ERingStage::EndCard);
				if (ARingPlayerController* Controller = Cast<ARingPlayerController>(GetWorld()->GetFirstPlayerController()))
				{
					Controller->ShowEndCard();
				}
			}
		}
		break;

	default:
		break;
	}
}

void ARingGameMode::TickAtmosphere(const FVector& L, float DeltaSeconds)
{
	const URingTuning& Tuning = URingTuning::Get();
	const bool bDark = L.X > 1930.0;
	const bool bLegTwo = L.X > 3880.0 && L.Y > 160.0;
	const bool bNearChamber = bLegTwo && L.Y > 900.0;
	const bool bPassive = FiredBeats.Contains(TEXT("passive")) && bLegTwo;

	// Suit light: on in the unpowered sector, off near the chamber so it cannot give her away.
	Player->SetSuitLight(bDark && !bNearChamber);
	// Strategic stays docked throughout; near the chamber even its indicator is dark.
	Player->SetDroneLit(!bPassive);

	float Brightness = 0.f;
	if (const URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		Brightness = Instance->Settings.Brightness;
	}
	DarkBias = FMath::FInterpTo(DarkBias, bDark ? Tuning.ExposureBiasDark : 0.f, DeltaSeconds, 1.f);
	Player->SetExposureBias(Brightness + DarkBias);

	if (URingSynthComponent* Synth = Player->GetSynth())
	{
		Synth->SetDarkness(bDark ? 1.f : 0.f);
		float Murmur = 0.f;
		float Tension = 0.f;
		if (Voraik && bDark)
		{
			const bool bSpeaking = !Voraik->HasStarted() || Voraik->IsSpeaking();
			if (bSpeaking && Stage <= ERingStage::Encounter)
			{
				const float Distance = static_cast<float>(FVector::Dist(L, Voraik->GetActorLocation()));
				const float Near = FMath::Clamp(1.f - Distance / MurmurRange, 0.f, 1.f);
				Murmur = Near * Near;
			}
			Tension = Voraik->IsInAwarePause() ? 1.f : 0.f;
		}
		Synth->SetMurmur(Murmur);
		Synth->SetTension(Tension);
	}
}

const FRingInteractPoint* ARingGameMode::FindInteract(const FVector& PlayerLocation) const
{
	const float Range = URingTuning::Get().InteractRange;
	const FRingInteractPoint* Best = nullptr;
	double BestDistance = static_cast<double>(Range);
	for (const FRingInteractPoint& Point : InteractPoints)
	{
		const double Distance = FVector::Dist2D(PlayerLocation, Point.Location);
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Best = &Point;
		}
	}
	return Best;
}

void ARingGameMode::HandleInteract()
{
	if (!Player || !bBuilt || Player->GetHeldObject() || IsTalking() || Stage >= ERingStage::Ending)
	{
		return;
	}
	const FRingInteractPoint* Point = FindInteract(Player->GetActorLocation());
	if (!Point)
	{
		return;
	}
	if (Point->Id == TEXT("lead"))
	{
		Say(SpeakerLead, TEXT("Go ahead. We'll finish securing the equipment."));
	}
	else if (Point->Id == TEXT("engineer"))
	{
		if (Stage <= ERingStage::OpenPartition)
		{
			Say(SpeakerEngineer, TEXT("The track on that partition looks sound. It's only the brace holding it."));
		}
		else
		{
			Say(SpeakerEngineer, TEXT("I'll have the cases sealed by the time you're back."));
		}
	}
	else if (Point->Id == TEXT("gap"))
	{
		Say(SpeakerStrategic, TEXT("The survey cache has this arc mapped to the limit of our instruments. It keeps going."));
	}
}

void ARingGameMode::UpdatePrompt(const FVector& PlayerLocation)
{
	Prompt.Reset();
	if (Stage >= ERingStage::Ending)
	{
		return;
	}
	bool bAimToggle = false;
	if (const URingGameInstance* Instance = GetGameInstance<URingGameInstance>())
	{
		bAimToggle = Instance->Settings.bAimToggle;
	}

	if (!Player->GetNotice().IsEmpty())
	{
		Prompt = Player->GetNotice();
		return;
	}
	if (const ARingTKObject* Held = Player->GetHeldObject())
	{
		if (Held->IsNearSnap())
		{
			Prompt = TEXT("Left mouse: set the panel into the support position");
		}
		else if (Held->IsTrack())
		{
			Prompt = TEXT("Look or walk along the track to slide it   |   Left mouse: let go");
		}
		else
		{
			Prompt = TEXT("Look and walk to move it   |   Mouse wheel: nearer / farther   |   Left mouse: let go");
		}
		return;
	}
	if (Player->GetFocusInput())
	{
		if (const ARingTKObject* Target = Player->GetTargetObject())
		{
			Prompt = Player->GetTargetBlockedReason().IsEmpty()
				? FString::Printf(TEXT("Left mouse: take hold of the %s"), *Target->GetDisplayName().ToLower())
				: Player->GetTargetBlockedReason();
		}
		else if (Stage >= ERingStage::ClearBrace && Stage <= ERingStage::PlaceBridge)
		{
			Prompt = TEXT("Aim at an object marked with two brass bands");
		}
		return;
	}
	if (const FRingInteractPoint* Point = FindInteract(PlayerLocation))
	{
		Prompt = FString::Printf(TEXT("E: %s"), *Point->Prompt);
		return;
	}
	if (Stage == ERingStage::Arrival || (Stage == ERingStage::ClearBrace && StageTime < 6.f && PlayerLocation.X < 200.0))
	{
		Prompt = TEXT("WASD: move   |   Mouse: look   |   Shift: brisk walk   |   E: interact");
	}
	else if (Stage >= ERingStage::ClearBrace && Stage <= ERingStage::PlaceBridge)
	{
		Prompt = bAimToggle ? TEXT("Right mouse: focus (press again to stop)") : TEXT("Hold right mouse: focus");
	}
}
