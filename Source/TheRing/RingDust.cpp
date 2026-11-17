#include "RingDust.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "RingBuild.h"

ARingDust::ARingDust()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ARingDust::Puff(UWorld* World, const FVector& Location, float InSpread)
{
	if (!World)
	{
		return;
	}
	if (ARingDust* Dust = World->SpawnActorDeferred<ARingDust>(ARingDust::StaticClass(), FTransform(Location), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Dust->Spread = InSpread;
		Dust->FinishSpawning(FTransform(Location));
	}
}

void ARingDust::BeginPlay()
{
	Super::BeginPlay();
	const FRingSurface DustSurface(FLinearColor(0.16f, 0.15f, 0.14f), 1.f, 0.f);
	FRandomStream Random(GetUniqueID());
	for (int32 Index = 0; Index < MoteCount; ++Index)
	{
		const FVector Offset(Random.FRandRange(-Spread, Spread), Random.FRandRange(-Spread, Spread), Random.FRandRange(0.f, 6.f));
		UStaticMeshComponent* Mote = FRingBuild::AddPart(this, RootComponent, ERingShape::Sphere, Offset, FVector(3.0), DustSurface);
		Mote->SetCastShadow(false);
		Motes.Add(Mote);
		Velocities.Add(FVector(Offset.X * 0.6, Offset.Y * 0.6, Random.FRandRange(8.f, 26.f)));
	}
}

void ARingDust::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Age >= Lifetime)
	{
		Destroy();
		return;
	}
	const float Alpha = Age / Lifetime;
	const float Size = FMath::Lerp(3.f, 0.2f, Alpha);
	for (int32 Index = 0; Index < Motes.Num(); ++Index)
	{
		if (UStaticMeshComponent* Mote = Motes[Index])
		{
			Mote->AddRelativeLocation(Velocities[Index] * DeltaSeconds * (1.f - Alpha));
			Mote->SetRelativeScale3D(FVector(Size / 100.0));
		}
	}
}
