#include "Misc/AutomationTest.h"
#include "RingLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RingLogicTestData
{
	const float HoldMin = 140.f;
	const float HoldMax = 520.f;
	const float HoldStep = 30.f;
	const float TrackLength = 300.f;
	const float SnapRadius = 130.f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRingStageAdvanceTest, "TheRing.Logic.StageAdvancesOneStepOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRingStageAdvanceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Arrival -> ClearBrace"), RingLogic::CanAdvance(ERingStage::Arrival, ERingStage::ClearBrace));
	TestTrue(TEXT("Encounter -> Retreat"), RingLogic::CanAdvance(ERingStage::Encounter, ERingStage::Retreat));
	TestFalse(TEXT("cannot skip a stage"), RingLogic::CanAdvance(ERingStage::Arrival, ERingStage::OpenPartition));
	TestFalse(TEXT("cannot go back"), RingLogic::CanAdvance(ERingStage::Retreat, ERingStage::Encounter));
	TestFalse(TEXT("cannot repeat a stage"), RingLogic::CanAdvance(ERingStage::Encounter, ERingStage::Encounter));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRingHoldDistanceTest, "TheRing.Logic.HoldDistanceStaysBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRingHoldDistanceTest::RunTest(const FString& Parameters)
{
	using namespace RingLogicTestData;
	TestEqual(TEXT("one step out"), RingLogic::AdjustHoldDistance(200.f, 1.f, HoldStep, HoldMin, HoldMax), 230.f);
	TestEqual(TEXT("clamped at max"), RingLogic::AdjustHoldDistance(510.f, 5.f, HoldStep, HoldMin, HoldMax), HoldMax);
	TestEqual(TEXT("clamped at min"), RingLogic::AdjustHoldDistance(150.f, -5.f, HoldStep, HoldMin, HoldMax), HoldMin);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRingTrackTest, "TheRing.Logic.PartitionStaysOnTrack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRingTrackTest::RunTest(const FString& Parameters)
{
	using namespace RingLogicTestData;
	const FVector Origin(630.0, 0.0, 159.0);
	const FVector Axis(0.0, 1.0, 0.0);
	TestEqual(TEXT("off-axis motion is ignored"), RingLogic::ProjectOnTrack(FVector(900.0, 120.0, 400.0), Origin, Axis, TrackLength), 120.f);
	TestEqual(TEXT("cannot pass the closed end"), RingLogic::ProjectOnTrack(FVector(630.0, -500.0, 159.0), Origin, Axis, TrackLength), 0.f);
	TestEqual(TEXT("cannot pass the open end"), RingLogic::ProjectOnTrack(FVector(630.0, 900.0, 159.0), Origin, Axis, TrackLength), TrackLength);
	TestEqual(TEXT("MoveTowards limits the step"), RingLogic::MoveTowards(0.f, 100.f, 10.f), 10.f);
	TestEqual(TEXT("MoveTowards does not overshoot"), RingLogic::MoveTowards(95.f, 100.f, 10.f), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRingSnapTest, "TheRing.Logic.SnapIsGenerousButBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRingSnapTest::RunTest(const FString& Parameters)
{
	using namespace RingLogicTestData;
	const FVector Snap(1275.0, 0.0, 6.0);
	TestTrue(TEXT("exact position snaps"), RingLogic::IsWithinSnap(Snap, Snap, SnapRadius));
	TestTrue(TEXT("held a metre above still snaps"), RingLogic::IsWithinSnap(Snap + FVector(40.0, 30.0, 100.0), Snap, SnapRadius));
	TestFalse(TEXT("two metres away does not snap"), RingLogic::IsWithinSnap(Snap + FVector(200.0, 0.0, 0.0), Snap, SnapRadius));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRingRampTest, "TheRing.Logic.RampAndSubtitleTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRingRampTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ramp is 0 before start"), RingLogic::Ramp(1.f, 2.f, 4.f), 0.f);
	TestEqual(TEXT("ramp is 1 after end"), RingLogic::Ramp(5.f, 2.f, 4.f), 1.f);
	TestEqual(TEXT("ramp midpoint"), RingLogic::Ramp(3.f, 2.f, 4.f), 0.5f);
	TestTrue(TEXT("short lines still get a readable minimum"), RingLogic::SubtitleDuration(4) >= 2.2f);
	TestTrue(TEXT("long lines get longer"), RingLogic::SubtitleDuration(120) > RingLogic::SubtitleDuration(20));
	return true;
}

#endif
