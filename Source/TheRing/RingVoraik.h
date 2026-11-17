#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RingVoraik.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The unfamiliar being in the dark chamber. "Voraik" is a development name
 * only: nothing in this class is shown to the player.
 *
 * PLACEHOLDER model: a simplified real-time stand-in assembled from basic
 * shapes and animated in code, built to the approved reference. One pair of
 * long support arms that reach the floor, one pair of small manipulation arms,
 * legs, a narrow flared head with a single vertical red line, mantle, cloak
 * and tabard, all matte black with no exposed flesh. Replace with the real
 * model when it is ready (see docs/the-ring/README.md).
 *
 * The encounter is a fixed timeline. He never reacts to the player: he prays
 * with his back to the doorway, lowers his arms, crosses to the far-wall
 * recess, takes a small object with a manipulation arm, pauses, turns his head
 * and upper body far enough to show his profile, holds, and turns back.
 * The actor's forward (+X) is the way he faces.
 */
UCLASS()
class THERING_API ARingVoraik : public AActor
{
	GENERATED_BODY()

public:
	ARingVoraik();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Where he walks to, and the object he takes there. Call before StartEncounter. */
	void SetRecess(const FVector& StandLocation, AActor* InShelfObject);
	/** Starts the timeline. Does nothing if it has already started. */
	void StartEncounter();
	bool HasStarted() const { return bStarted; }
	/** Seconds since the encounter started; 0 before. */
	float GetEncounterTime() const { return Time; }
	bool IsSpeaking() const { return bStarted && Time < SpeechEnd; }
	/** True from the moment he stops moving after the object is taken until he has turned back. */
	bool IsInAwarePause() const { return bStarted && Time >= PauseStart && Time < TurnBackEnd; }
	bool IsEncounterComplete() const { return bStarted && Time >= CompleteTime; }

	// Timeline, seconds from StartEncounter. SpeechEnd is set from URingTuning::PrayDuration.
	float SpeechEnd = 18.f;
	float ArmsDownEnd = 22.f;
	float WalkStart = 24.f;
	float WalkEnd = 32.f;
	float ReachEnd = 35.f;
	float TakeEnd = 36.f;
	float RetractEnd = 38.5f;
	float PauseStart = 38.5f;
	float TurnStart = 41.f;
	float TurnEnd = 44.5f;
	float HoldEnd = 49.f;
	float TurnBackEnd = 53.f;
	float CompleteTime = 55.f;

private:
	void BuildBody();
	void ApplyPose(float DeltaSeconds);

	UPROPERTY() TObjectPtr<USceneComponent> Upper;
	UPROPERTY() TObjectPtr<USceneComponent> Head;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LongUpper[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LongLower[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LongTip[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LongCollar[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Drape[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SmallUpper[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SmallLower[2];
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SmallHand[2];
	UPROPERTY() TObjectPtr<UPointLightComponent> FaceGlow;
	UPROPERTY() TObjectPtr<AActor> ShelfObject;

	bool bStarted = false;
	bool bObjectTaken = false;
	float Time = 0.f;
	float Clock = 0.f;
	FVector StartLocation = FVector::ZeroVector;
	FVector RecessLocation = FVector::ZeroVector;
	FVector ShelfObjectStart = FVector::ZeroVector;
};
