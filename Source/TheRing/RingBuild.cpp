#include "RingBuild.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** Optional project materials, made by Tools/Editor/create_ring_materials.py. */
	const TCHAR* ProjectSurfacePath = TEXT("/Game/TheRing/Materials/M_RingSurface.M_RingSurface");
	const TCHAR* ProjectEmissivePath = TEXT("/Game/TheRing/Materials/M_RingEmissive.M_RingEmissive");
	/** Engine fallbacks. */
	const TCHAR* EngineSurfacePath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	const TCHAR* EngineEmissivePath = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	UMaterialInterface* LoadQuiet(const TCHAR* Path)
	{
		return Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, Path, {}, LOAD_NoWarn | LOAD_Quiet));
	}

	UMaterialInterface* SurfaceMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			UMaterialInterface* Found = LoadQuiet(ProjectSurfacePath);
			if (!Found)
			{
				Found = LoadQuiet(EngineSurfacePath);
			}
			Cached = Found;
		}
		return Cached.Get();
	}

	UMaterialInterface* EmissiveMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		static bool bSearched = false;
		if (!Cached.IsValid() && !bSearched)
		{
			bSearched = true;
			UMaterialInterface* Found = LoadQuiet(ProjectEmissivePath);
			if (!Found)
			{
				Found = LoadQuiet(EngineEmissivePath);
			}
			Cached = Found;
		}
		return Cached.Get();
	}

	FActorSpawnParameters AlwaysSpawn()
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return Params;
	}
}

UStaticMesh* FRingBuild::Mesh(ERingShape Shape)
{
	const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
	switch (Shape)
	{
	case ERingShape::Sphere: Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
	case ERingShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
	case ERingShape::Cone: Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
	default: break;
	}
	return LoadObject<UStaticMesh>(nullptr, Path);
}

bool FRingBuild::HasEmissive()
{
	return EmissiveMaterial() != nullptr;
}

UMaterialInstanceDynamic* FRingBuild::MakeMaterial(UObject* Outer, const FRingSurface& Surface)
{
	UMaterialInterface* Base = (Surface.Emissive > 0.f && EmissiveMaterial()) ? EmissiveMaterial() : SurfaceMaterial();
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
	if (Material)
	{
		// Parameters a material does not have are ignored, so one call set serves every base material.
		Material->SetVectorParameterValue(TEXT("Color"), Surface.Color);
		Material->SetScalarParameterValue(TEXT("Roughness"), Surface.Roughness);
		Material->SetScalarParameterValue(TEXT("Metallic"), Surface.Metallic);
		Material->SetScalarParameterValue(TEXT("Strength"), Surface.Emissive);
	}
	return Material;
}

UStaticMeshComponent* FRingBuild::AddPart(AActor* Owner, USceneComponent* Parent, ERingShape Shape, const FVector& RelativeLocation,
	const FVector& SizeCm, const FRingSurface& Surface, const FRotator& RelativeRotation)
{
	check(Owner);
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetStaticMesh(Mesh(Shape));
	if (UMaterialInstanceDynamic* Material = MakeMaterial(Part, Surface))
	{
		Part->SetMaterial(0, Material);
	}
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCanEverAffectNavigation(false);
	Part->SetupAttachment(Parent);
	Part->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
	Part->SetRelativeScale3D(SizeCm / 100.0);
	Part->RegisterComponent();
	Owner->AddInstanceComponent(Part);
	return Part;
}

USceneComponent* FRingBuild::AddPivot(AActor* Owner, USceneComponent* Parent, const FVector& RelativeLocation)
{
	check(Owner);
	USceneComponent* Pivot = NewObject<USceneComponent>(Owner);
	Pivot->SetMobility(EComponentMobility::Movable);
	Pivot->SetupAttachment(Parent);
	Pivot->SetRelativeLocation(RelativeLocation);
	Pivot->RegisterComponent();
	Owner->AddInstanceComponent(Pivot);
	return Pivot;
}

AStaticMeshActor* FRingBuild::Block(UWorld* World, const FVector& Center, const FVector& SizeCm, const FRingSurface& Surface,
	const FRotator& Rotation, bool bCollide, bool bCastShadow, ERingShape Shape)
{
	check(World);
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, Center), AlwaysSpawn());
	if (!Actor)
	{
		return nullptr;
	}
	UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(Mesh(Shape));
	if (UMaterialInstanceDynamic* Material = MakeMaterial(Component, Surface))
	{
		Component->SetMaterial(0, Material);
	}
	Actor->SetActorScale3D(SizeCm / 100.0);
	if (bCollide)
	{
		Component->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Component->SetCastShadow(bCastShadow);
	Component->SetCanEverAffectNavigation(false);
	return Actor;
}

AStaticMeshActor* FRingBuild::Slab(UWorld* World, const FVector& Min, const FVector& Max, const FRingSurface& Surface, bool bCollide)
{
	return Block(World, (Min + Max) * 0.5, (Max - Min).GetAbs(), Surface, FRotator::ZeroRotator, bCollide, true, ERingShape::Cube);
}

AStaticMeshActor* FRingBuild::PawnBlocker(UWorld* World, const FVector& Min, const FVector& Max)
{
	AStaticMeshActor* Actor = Slab(World, Min, Max, FRingSurface(), true);
	if (Actor)
	{
		UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		Actor->SetActorHiddenInGame(true);
		Component->SetCastShadow(false);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	return Actor;
}

APointLight* FRingBuild::PointLight(UWorld* World, const FVector& Location, float Candela, const FLinearColor& Color, float Radius, bool bShadows)
{
	check(World);
	APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(Location), AlwaysSpawn());
	if (Light)
	{
		if (UPointLightComponent* Component = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetIntensityUnits(ELightUnits::Candelas);
			Component->SetIntensity(Candela);
			Component->SetLightColor(Color);
			Component->SetAttenuationRadius(Radius);
			Component->SetCastShadows(bShadows);
		}
	}
	return Light;
}

ASpotLight* FRingBuild::SpotLight(UWorld* World, const FVector& Location, const FRotator& Rotation, float Candela, const FLinearColor& Color,
	float Radius, float InnerCone, float OuterCone, bool bShadows)
{
	check(World);
	ASpotLight* Light = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), FTransform(Rotation, Location), AlwaysSpawn());
	if (Light)
	{
		if (USpotLightComponent* Component = Cast<USpotLightComponent>(Light->GetLightComponent()))
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetIntensityUnits(ELightUnits::Candelas);
			Component->SetIntensity(Candela);
			Component->SetLightColor(Color);
			Component->SetAttenuationRadius(Radius);
			Component->SetInnerConeAngle(InnerCone);
			Component->SetOuterConeAngle(OuterCone);
			Component->SetCastShadows(bShadows);
		}
	}
	return Light;
}

ADirectionalLight* FRingBuild::StarLight(UWorld* World, const FRotator& Rotation, float Lux, const FLinearColor& Color)
{
	check(World);
	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(Rotation, FVector::ZeroVector), AlwaysSpawn());
	if (Light)
	{
		if (ULightComponent* Component = Light->GetLightComponent())
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetIntensity(Lux);
			Component->SetLightColor(Color);
			Component->SetCastShadows(true);
		}
	}
	return Light;
}

void FRingBuild::PlaceSegment(USceneComponent* Part, const FVector& Start, const FVector& Direction, float Length)
{
	if (!Part)
	{
		return;
	}
	const FVector Dir = Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	Part->SetRelativeLocationAndRotation(Start + Dir * (Length * 0.5), FQuat::FindBetweenNormals(FVector::UpVector, Dir));
}
