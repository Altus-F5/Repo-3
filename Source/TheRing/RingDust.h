#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RingDust.generated.h"

class UStaticMeshComponent;

/** A small, short-lived puff of dust motes. Built from tiny meshes so it needs no particle asset. */
UCLASS()
class THERING_API ARingDust : public AActor
{
	GENERATED_BODY()

public:
	ARingDust();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Spawns a puff at Location spreading over roughly Spread centimetres. */
	static void Puff(UWorld* World, const FVector& Location, float Spread);

private:
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Motes;
	TArray<FVector> Velocities;
	float Age = 0.f;
	float Spread = 40.f;
	static constexpr float Lifetime = 1.4f;
	static constexpr int32 MoteCount = 9;
};
