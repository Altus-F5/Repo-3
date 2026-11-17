#pragma once

#include "CoreMinimal.h"

class AActor;
class APointLight;
class ASpotLight;
class ADirectionalLight;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWorld;

/** The engine basic shapes everything in the demo is assembled from. */
enum class ERingShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone
};

/** Surface description. Roughness, Metallic and Emissive only take effect with the optional project materials. */
struct FRingSurface
{
	FLinearColor Color = FLinearColor(0.05f, 0.05f, 0.055f);
	float Roughness = 0.7f;
	float Metallic = 0.1f;
	float Emissive = 0.f;

	FRingSurface() {}
	FRingSurface(const FLinearColor& InColor, float InRoughness = 0.7f, float InMetallic = 0.1f, float InEmissive = 0.f)
		: Color(InColor), Roughness(InRoughness), Metallic(InMetallic), Emissive(InEmissive)
	{
	}
};

/**
 * Helpers that assemble the level and the characters from engine basic shapes,
 * so the project needs no imported assets.
 */
class THERING_API FRingBuild
{
public:
	/** Engine basic-shape mesh. Each is 100 units across. */
	static UStaticMesh* Mesh(ERingShape Shape);

	/** True when an emissive material was found (the project one, or an engine fallback). */
	static bool HasEmissive();

	/** Dynamic material for a surface. */
	static UMaterialInstanceDynamic* MakeMaterial(UObject* Outer, const FRingSurface& Surface);

	/** Adds a non-colliding mesh part to an actor. SizeCm is the full size in centimetres. */
	static UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, ERingShape Shape, const FVector& RelativeLocation,
		const FVector& SizeCm, const FRingSurface& Surface, const FRotator& RelativeRotation = FRotator::ZeroRotator);

	/** Adds an empty pivot to an actor. */
	static USceneComponent* AddPivot(AActor* Owner, USceneComponent* Parent, const FVector& RelativeLocation);

	/** Spawns a world block centred on Center. */
	static AStaticMeshActor* Block(UWorld* World, const FVector& Center, const FVector& SizeCm, const FRingSurface& Surface,
		const FRotator& Rotation = FRotator::ZeroRotator, bool bCollide = true, bool bCastShadow = true, ERingShape Shape = ERingShape::Cube);

	/** Spawns an axis-aligned block from two opposite corners. */
	static AStaticMeshActor* Slab(UWorld* World, const FVector& Min, const FVector& Max, const FRingSurface& Surface, bool bCollide = true);

	/** Spawns an invisible volume that stops the player and nothing else. */
	static AStaticMeshActor* PawnBlocker(UWorld* World, const FVector& Min, const FVector& Max);

	static APointLight* PointLight(UWorld* World, const FVector& Location, float Candela, const FLinearColor& Color, float Radius, bool bShadows);
	static ASpotLight* SpotLight(UWorld* World, const FVector& Location, const FRotator& Rotation, float Candela, const FLinearColor& Color,
		float Radius, float InnerCone, float OuterCone, bool bShadows);
	static ADirectionalLight* StarLight(UWorld* World, const FRotator& Rotation, float Lux, const FLinearColor& Color);

	/** Points a cylinder (or any Z-aligned part) from Start along Direction for Length. */
	static void PlaceSegment(USceneComponent* Part, const FVector& Start, const FVector& Direction, float Length);
};
