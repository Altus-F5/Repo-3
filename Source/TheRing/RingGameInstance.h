#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RingTypes.h"
#include "RingGameInstance.generated.h"

/**
 * Holds what must survive a level reload: the options, and whether the next
 * load should start from the checkpoint. Restart works by reloading the map,
 * and the whole level is rebuilt in code, so nothing else can carry over.
 */
UCLASS()
class THERING_API URingGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** Writes the options and checkpoint flag to the save slot. */
	void SaveToDisk() const;

	/** Records the checkpoint and saves it. */
	void MarkCheckpoint();

	/** Forgets the checkpoint (used when the demo is replayed from the start). */
	void ClearCheckpoint();

	UPROPERTY() FRingSettings Settings;
	/** The checkpoint before the encounter has been reached at least once. */
	UPROPERTY() bool bCheckpointReached = false;
	/** The level being loaded should start at the checkpoint. Consumed by the game mode. */
	UPROPERTY() bool bStartFromCheckpoint = false;

private:
	static const TCHAR* SlotName() { return TEXT("TheRingDemo"); }
};
