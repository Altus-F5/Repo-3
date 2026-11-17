#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RingBodyComponent.h"
#include "RingTypes.h"
#include "RingGameMode.generated.h"

class ARingCharacter;
class ARingTKObject;
class ARingVoraik;
class AStaticMeshActor;

/** One subtitle line. An empty Speaker makes it a sound caption. */
struct FRingLine
{
	FString Speaker;
	FString Text;
	float Duration = 0.f;
};

/** Something the player can use with the ordinary interact button. */
struct FRingInteractPoint
{
	FName Id;
	FVector Location = FVector::ZeroVector;
	FString Prompt;
};

/**
 * The demo's director. It builds the whole level in code when play starts, then
 * runs the progression as a small state machine (ERingStage) that only ever
 * advances one stage at a time, so a trigger cannot fire twice. It owns the
 * dialogue queue, the checkpoint and the puzzle reset.
 *
 * It ticks only while the game is unpaused, so pausing stops dialogue timing
 * and story progression together.
 */
UCLASS()
class THERING_API ARingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARingGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The ordinary interact button was pressed. */
	void HandleInteract();
	/** Puts the telekinesis objects back so the puzzles can be redone, without ever closing the route behind the player. */
	void ResetPuzzleObjects();
	/** Reloads the level at the checkpoint (or at the start if it has not been reached). */
	void RestartFromCheckpoint();
	/** Forgets the checkpoint and reloads the level from the start. */
	void RestartDemo();

	ERingStage GetStage() const { return Stage; }

	// --- Read by the HUD ---
	const FString& GetObjective() const { return Objective; }
	const FString& GetPrompt() const { return Prompt; }
	const FString& GetToast() const { return Toast; }
	/** False when nothing is being said. */
	bool GetSubtitle(FString& OutSpeaker, FString& OutText) const;
	/** 0 clear .. 1 black. */
	float GetFade() const { return Fade; }
	/** Where the support position for the bridge panel is, for the HUD marker. */
	bool GetSnapMarker(FVector& OutLocation) const;

private:
	void BuildLevel();
	void BuildStaging();
	void BuildAccessHall();
	void BuildCorridor();
	void BuildChamber();
	void BuildVista();
	AActor* SpawnResearcher(const FVector& Location, float Yaw, ERingBodyLook Look, ERingIdleStyle Style, float Scale);

	void PlacePlayer(bool bFromCheckpoint);
	void Advance(ERingStage To);
	void UpdateObjective();
	void TickStage(const FVector& PlayerLocation, float DeltaSeconds);
	void TickAtmosphere(const FVector& PlayerLocation, float DeltaSeconds);
	void TickDialogue(float DeltaSeconds);
	void UpdatePrompt(const FVector& PlayerLocation);
	void Say(const FString& Speaker, const FString& Text, float Duration = 0.f);
	bool IsTalking() const { return Lines.Num() > 0; }
	/** True the first time it is called with a given name, false afterwards. */
	bool Once(FName Beat);
	const FRingInteractPoint* FindInteract(const FVector& PlayerLocation) const;
	void ShowToast(const FString& Text);
	void Reload();

	UPROPERTY() TObjectPtr<ARingCharacter> Player;
	UPROPERTY() TObjectPtr<ARingTKObject> Bar;
	UPROPERTY() TObjectPtr<ARingTKObject> Partition;
	UPROPERTY() TObjectPtr<ARingTKObject> Panel;
	UPROPERTY() TObjectPtr<ARingVoraik> Voraik;
	UPROPERTY() TObjectPtr<AStaticMeshActor> GapBlocker;
	UPROPERTY() TObjectPtr<AActor> SurveyLead;
	UPROPERTY() TObjectPtr<AActor> Engineer;

	ERingStage Stage = ERingStage::Arrival;
	float StageTime = 0.f;
	TSet<FName> FiredBeats;
	TArray<FRingLine> Lines;
	float LineTime = 0.f;
	TArray<FRingInteractPoint> InteractPoints;

	FString Objective;
	FString Prompt;
	FString Toast;
	float ToastTime = 0.f;
	float Fade = 1.f;
	float FadeTarget = 0.f;
	float EndingQuiet = 0.f;
	float DarkBias = 0.f;
	bool bBuilt = false;
	bool bBridgeOpen = false;
	FVector LastSafeLocation = FVector::ZeroVector;
};
