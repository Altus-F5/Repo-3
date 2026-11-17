#include "RingGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "RingSaveGame.h"

void URingGameInstance::Init()
{
	Super::Init();
	if (UGameplayStatics::DoesSaveGameExist(SlotName(), 0))
	{
		if (const URingSaveGame* Save = Cast<URingSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
		{
			Settings = Save->Settings;
			bCheckpointReached = Save->bCheckpointReached;
		}
	}
}

void URingGameInstance::SaveToDisk() const
{
	if (URingSaveGame* Save = Cast<URingSaveGame>(UGameplayStatics::CreateSaveGameObject(URingSaveGame::StaticClass())))
	{
		Save->Settings = Settings;
		Save->bCheckpointReached = bCheckpointReached;
		UGameplayStatics::SaveGameToSlot(Save, SlotName(), 0);
	}
}

void URingGameInstance::MarkCheckpoint()
{
	bCheckpointReached = true;
	SaveToDisk();
}

void URingGameInstance::ClearCheckpoint()
{
	bCheckpointReached = false;
	bStartFromCheckpoint = false;
	SaveToDisk();
}
