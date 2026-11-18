#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCCityGenerator.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Procedural grid city. All static geometry is batched into a handful of HISM components
 * (one draw call per layer + per-instance culling), which is what keeps thousands of
 * buildings cheap. Also provides the road-grid navigation used by AI.
 *
 * Layout: intersections sit at (i * Pitch, j * Pitch) for i, j in [0, GridSize].
 * Block (bx, by) is the square between intersections (bx, by) and (bx + 1, by + 1).
 */
UCLASS()
class OPENCITY_API AOCCityGenerator : public AActor
{
	GENERATED_BODY()

public:
	AOCCityGenerator();

	UPROPERTY(EditAnywhere, Category = "City") int32 GridSize = 14;
	UPROPERTY(EditAnywhere, Category = "City") float BlockSize = 6000.f;
	UPROPERTY(EditAnywhere, Category = "City") float RoadWidth = 1400.f;
	UPROPERTY(EditAnywhere, Category = "City") float SidewalkWidth = 400.f;
	UPROPERTY(EditAnywhere, Category = "City") int32 Seed = 20261118;
	UPROPERTY(EditAnywhere, Category = "City") float ParkChance = 0.1f;
	UPROPERTY(EditAnywhere, Category = "City") float ParkedCarChance = 0.45f;
	UPROPERTY(EditAnywhere, Category = "City") FIntPoint SpawnBlock = FIntPoint(3, 3);

	void Generate();
	bool IsGenerated() const { return bGenerated; }

	float GetPitch() const { return BlockSize + RoadWidth; }
	float GetExtent() const { return GridSize * GetPitch(); }
	float GetLaneOffset() const { return RoadWidth * 0.24f; }
	float GetSidewalkTopZ() const { return 15.f; }

	bool IsValidIntersection(FIntPoint I) const { return I.X >= 0 && I.Y >= 0 && I.X <= GridSize && I.Y <= GridSize; }
	bool IsValidBlock(FIntPoint B) const { return B.X >= 0 && B.Y >= 0 && B.X < GridSize && B.Y < GridSize; }

	FVector GetIntersectionLocation(FIntPoint I) const;
	FIntPoint GetNearestIntersection(const FVector& Location) const;
	FVector GetSidewalkCorner(FIntPoint Block, int32 Corner) const;
	void FindNearestSidewalkCorner(const FVector& Location, FIntPoint& OutBlock, int32& OutCorner) const;
	bool FindSidewalkSpawn(const FVector& Origin, float MinDist, float MaxDist, FIntPoint& OutBlock, int32& OutCorner) const;
	bool FindLaneSpawn(const FVector& Origin, float MinDist, float MaxDist, FTransform& OutTransform, FIntPoint& OutNextIntersection, FIntPoint& OutDirection) const;

	/** Next point to walk towards: straight line if clear, otherwise along the road grid. */
	FVector GetNavWaypoint(const FVector& From, const FVector& To) const;

	FTransform GetPlayerSpawnTransform() const;

private:
	UHierarchicalInstancedStaticMeshComponent* MakeLayer(const FName Name, UStaticMesh* Mesh, bool bCollision, float CullDistance, bool bShadows = true);
	void AddInstance(UHierarchicalInstancedStaticMeshComponent* Layer, const FVector& Center, const FVector& Size);
	void BuildRoads();
	void BuildBlock(FIntPoint Block, FRandomStream& Rng);
	void SpawnParkedCars(FRandomStream& Rng);
	bool IsPathClear(const FVector& A, const FVector& B) const;

	UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> BuildingLayers;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> MarkingLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SidewalkLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoofLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LampLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TrunkLayer;
	UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CanopyLayer;

	TSet<FIntPoint> Parks;
	bool bGenerated = false;
};
