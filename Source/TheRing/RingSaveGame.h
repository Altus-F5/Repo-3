#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RingTypes.h"
#include "RingSaveGame.generated.h"

/** What the demo writes to disk: the options, and whether the checkpoint has been reached. */
UCLASS()
class THERING_API URingSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() FRingSettings Settings;
	UPROPERTY() bool bCheckpointReached = false;
};
