#include "OCCityGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "OCAssets.h"
#include "OCVehicle.h"

namespace
{
	const FLinearColor BuildingColors[] = {
		FLinearColor(0.55f, 0.53f, 0.50f), // concrete
		FLinearColor(0.10f, 0.18f, 0.28f), // blue glass
		FLinearColor(0.38f, 0.16f, 0.10f), // brick
		FLinearColor(0.68f, 0.60f, 0.46f), // sandstone
		FLinearColor(0.06f, 0.06f, 0.07f), // dark glass
		FLinearColor(0.82f, 0.82f, 0.80f)  // white render
	};
}

AOCCityGenerator::AOCCityGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	GroundLayer = MakeLayer(TEXT("Ground"), OCAssets::Cube(), true, 0.f, false);
	RoadLayer = MakeLayer(TEXT("Roads"), OCAssets::Cube(), true, 0.f, false);
	MarkingLayer = MakeLayer(TEXT("Markings"), OCAssets::Cube(), false, 15000.f, false);
	SidewalkLayer = MakeLayer(TEXT("Sidewalks"), OCAssets::Cube(), true, 0.f);
	GrassLayer = MakeLayer(TEXT("Grass"), OCAssets::Cube(), true, 0.f, false);
	RoofLayer = MakeLayer(TEXT("RoofUnits"), OCAssets::Cube(), true, 40000.f);
	LampLayer = MakeLayer(TEXT("StreetLamps"), OCAssets::Cylinder(), true, 20000.f);
	TrunkLayer = MakeLayer(TEXT("TreeTrunks"), OCAssets::Cylinder(), true, 25000.f);
	CanopyLayer = MakeLayer(TEXT("TreeCanopies"), OCAssets::Sphere(), false, 30000.f);

	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(BuildingColors)); ++i)
	{
		BuildingLayers.Add(MakeLayer(FName(*FString::Printf(TEXT("Buildings%d"), i)), OCAssets::Cube(), true, 0.f));
	}
}

UHierarchicalInstancedStaticMeshComponent* AOCCityGenerator::MakeLayer(const FName Name, UStaticMesh* Mesh, bool bCollision, float CullDistance, bool bShadows)
{
	UHierarchicalInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
	Layer->SetupAttachment(RootComponent);
	Layer->SetStaticMesh(Mesh);
	Layer->SetMobility(EComponentMobility::Static);
	Layer->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Layer->SetCastShadow(bShadows);
	if (CullDistance > 0.f)
	{
		Layer->InstanceStartCullDistance = static_cast<int32>(CullDistance * 0.8f);
		Layer->InstanceEndCullDistance = static_cast<int32>(CullDistance);
	}
	return Layer;
}

void AOCCityGenerator::AddInstance(UHierarchicalInstancedStaticMeshComponent* Layer, const FVector& Center, const FVector& Size)
{
	Layer->AddInstance(FTransform(FRotator::ZeroRotator, Center, Size / 100.f));
}

void AOCCityGenerator::Generate()
{
	if (bGenerated)
	{
		return;
	}
	bGenerated = true;

	FRandomStream Rng(Seed);

	OCAssets::Paint(GroundLayer, FLinearColor(0.09f, 0.14f, 0.06f));
	OCAssets::Paint(RoadLayer, FLinearColor(0.03f, 0.03f, 0.035f));
	OCAssets::Paint(MarkingLayer, FLinearColor(0.9f, 0.82f, 0.55f));
	OCAssets::Paint(SidewalkLayer, FLinearColor(0.33f, 0.32f, 0.30f));
	OCAssets::Paint(GrassLayer, FLinearColor(0.11f, 0.24f, 0.06f));
	OCAssets::Paint(RoofLayer, FLinearColor(0.18f, 0.18f, 0.19f));
	OCAssets::Paint(LampLayer, FLinearColor(0.12f, 0.12f, 0.13f));
	OCAssets::Paint(TrunkLayer, FLinearColor(0.22f, 0.13f, 0.07f));
	OCAssets::Paint(CanopyLayer, FLinearColor(0.07f, 0.20f, 0.05f));
	for (int32 i = 0; i < BuildingLayers.Num(); ++i)
	{
		OCAssets::Paint(BuildingLayers[i], BuildingColors[i]);
	}

	for (int32 X = 0; X < GridSize; ++X)
	{
		for (int32 Y = 0; Y < GridSize; ++Y)
		{
			const FIntPoint Block(X, Y);
			if (Block != SpawnBlock && Rng.FRand() < ParkChance)
			{
				Parks.Add(Block);
			}
		}
	}

	BuildRoads();
	for (int32 X = 0; X < GridSize; ++X)
	{
		for (int32 Y = 0; Y < GridSize; ++Y)
		{
			BuildBlock(FIntPoint(X, Y), Rng);
		}
	}
	SpawnParkedCars(Rng);
}

void AOCCityGenerator::BuildRoads()
{
	const float P = GetPitch();
	const float E = GetExtent();
	const float R = RoadWidth;

	AddInstance(GroundLayer, FVector(E * 0.5f, E * 0.5f, -55.f), FVector(E + 60000.f, E + 60000.f, 100.f));

	for (int32 i = 0; i <= GridSize; ++i)
	{
		const float C = i * P;
		AddInstance(RoadLayer, FVector(C, E * 0.5f, -10.f), FVector(R, E + R, 20.f));
		AddInstance(RoadLayer, FVector(E * 0.5f, C, -10.f), FVector(E + R, R, 20.f));

		for (int32 j = 0; j < GridSize; ++j)
		{
			const float SegStart = j * P + R * 0.5f + 200.f;
			const float SegEnd = (j + 1) * P - R * 0.5f - 200.f;
			for (float T = SegStart; T + 300.f < SegEnd; T += 700.f)
			{
				AddInstance(MarkingLayer, FVector(C, T + 150.f, 1.f), FVector(18.f, 300.f, 2.f));
				AddInstance(MarkingLayer, FVector(T + 150.f, C, 1.f), FVector(300.f, 18.f, 2.f));
			}
		}
	}
}

void AOCCityGenerator::BuildBlock(FIntPoint Block, FRandomStream& Rng)
{
	const float P = GetPitch();
	const FVector2D Min(Block.X * P + RoadWidth * 0.5f, Block.Y * P + RoadWidth * 0.5f);
	const FVector2D Max = Min + FVector2D(BlockSize);
	const FVector2D Center = (Min + Max) * 0.5f;
	const float Top = GetSidewalkTopZ();

	AddInstance(SidewalkLayer, FVector(Center, Top - 25.f), FVector(BlockSize, BlockSize, 50.f));

	// Street lamps along the curb, clear of the pedestrian walking line.
	const float LampInset = 70.f;
	const FVector2D LampMin = Min + FVector2D(LampInset);
	const FVector2D LampMax = Max - FVector2D(LampInset);
	for (const float F : { 0.2f, 0.5f, 0.8f })
	{
		const float LX = FMath::Lerp(LampMin.X, LampMax.X, F);
		const float LY = FMath::Lerp(LampMin.Y, LampMax.Y, F);
		for (const FVector2D& Pos : { FVector2D(LX, LampMin.Y), FVector2D(LX, LampMax.Y), FVector2D(LampMin.X, LY), FVector2D(LampMax.X, LY) })
		{
			AddInstance(LampLayer, FVector(Pos, Top + 300.f), FVector(20.f, 20.f, 600.f));
		}
	}

	const FVector2D InnerMin = Min + FVector2D(SidewalkWidth);
	const FVector2D InnerMax = Max - FVector2D(SidewalkWidth);

	if (Parks.Contains(Block))
	{
		AddInstance(GrassLayer, FVector(Center, Top + 5.f), FVector(InnerMax - InnerMin, 20.f));
		const int32 TreeCount = Rng.RandRange(10, 18);
		for (int32 i = 0; i < TreeCount; ++i)
		{
			const FVector2D Pos(Rng.FRandRange(InnerMin.X + 300.f, InnerMax.X - 300.f), Rng.FRandRange(InnerMin.Y + 300.f, InnerMax.Y - 300.f));
			const float TrunkHeight = Rng.FRandRange(250.f, 420.f);
			const float CanopySize = Rng.FRandRange(350.f, 560.f);
			AddInstance(TrunkLayer, FVector(Pos, Top + 15.f + TrunkHeight * 0.5f), FVector(35.f, 35.f, TrunkHeight));
			AddInstance(CanopyLayer, FVector(Pos, Top + 15.f + TrunkHeight + CanopySize * 0.35f), FVector(CanopySize));
		}
		return;
	}

	const FVector2D CityCenter(GetExtent() * 0.5f, GetExtent() * 0.5f);
	const float Downtown = 1.f - FMath::Clamp(FVector2D::Distance(Center, CityCenter) / (GetExtent() * 0.55f), 0.f, 1.f);

	const int32 DivX = Rng.RandRange(1, 3);
	const int32 DivY = Rng.RandRange(1, 3);
	const FVector2D LotSize = (InnerMax - InnerMin) / FVector2D(DivX, DivY);

	for (int32 LX = 0; LX < DivX; ++LX)
	{
		for (int32 LY = 0; LY < DivY; ++LY)
		{
			const FVector2D LotMin = InnerMin + LotSize * FVector2D(LX, LY);
			const FVector2D LotCenter = LotMin + LotSize * 0.5f;
			const float Margin = Rng.FRandRange(80.f, 260.f);
			FVector2D Footprint = LotSize - FVector2D(Margin * 2.f);
			if (Footprint.X < 600.f || Footprint.Y < 600.f)
			{
				continue;
			}
			Footprint *= FVector2D(Rng.FRandRange(0.8f, 1.f), Rng.FRandRange(0.8f, 1.f));

			float Height = Rng.FRandRange(900.f, 3200.f) + FMath::Square(Downtown) * Rng.FRandRange(4000.f, 24000.f);
			if (Downtown > 0.6f && Rng.FRand() < 0.3f)
			{
				Height *= 1.5f;
			}

			const int32 Style = Rng.RandRange(0, BuildingLayers.Num() - 1);
			AddInstance(BuildingLayers[Style], FVector(LotCenter, Top + Height * 0.5f), FVector(Footprint, Height));

			if (Height > 6000.f && Rng.FRand() < 0.6f)
			{
				const FVector2D TowerFoot = Footprint * Rng.FRandRange(0.5f, 0.75f);
				const float TowerHeight = Height * Rng.FRandRange(0.2f, 0.45f);
				AddInstance(BuildingLayers[(Style + 1) % BuildingLayers.Num()], FVector(LotCenter, Top + Height + TowerHeight * 0.5f), FVector(TowerFoot, TowerHeight));
				Height += TowerHeight;
			}

			if (Rng.FRand() < 0.7f)
			{
				const FVector2D Offset(Rng.FRandRange(-0.2f, 0.2f) * Footprint.X, Rng.FRandRange(-0.2f, 0.2f) * Footprint.Y);
				AddInstance(RoofLayer, FVector(LotCenter + Offset, Top + Height + 150.f), FVector(Footprint * 0.25f, 300.f));
			}
		}
	}
}

void AOCCityGenerator::SpawnParkedCars(FRandomStream& Rng)
{
	UWorld* World = GetWorld();
	const float P = GetPitch();
	const float CurbOffset = 130.f;
	const float Z = AOCVehicle::GetRestHeight();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 X = 0; X < GridSize; ++X)
	{
		for (int32 Y = 0; Y < GridSize; ++Y)
		{
			if (Rng.FRand() > ParkedCarChance)
			{
				continue;
			}
			const FVector2D Min(X * P + RoadWidth * 0.5f, Y * P + RoadWidth * 0.5f);
			const FVector2D Max = Min + FVector2D(BlockSize);
			const float T = Rng.FRandRange(0.25f, 0.75f);

			FVector Location;
			float Yaw = 0.f;
			switch (Rng.RandRange(0, 3))
			{
			case 0: Location = FVector(FMath::Lerp(Min.X, Max.X, T), Min.Y - CurbOffset, Z); Yaw = 0.f; break;
			case 1: Location = FVector(Max.X + CurbOffset, FMath::Lerp(Min.Y, Max.Y, T), Z); Yaw = 90.f; break;
			case 2: Location = FVector(FMath::Lerp(Min.X, Max.X, T), Max.Y + CurbOffset, Z); Yaw = 180.f; break;
			default: Location = FVector(Min.X - CurbOffset, FMath::Lerp(Min.Y, Max.Y, T), Z); Yaw = -90.f; break;
			}

			World->SpawnActor<AOCVehicle>(AOCVehicle::StaticClass(), Location, FRotator(0.f, Yaw, 0.f), SpawnParams);
		}
	}
}

FVector AOCCityGenerator::GetIntersectionLocation(FIntPoint I) const
{
	I.X = FMath::Clamp(I.X, 0, GridSize);
	I.Y = FMath::Clamp(I.Y, 0, GridSize);
	return FVector(I.X * GetPitch(), I.Y * GetPitch(), 0.f);
}

FIntPoint AOCCityGenerator::GetNearestIntersection(const FVector& Location) const
{
	const float P = GetPitch();
	return FIntPoint(
		FMath::Clamp(FMath::RoundToInt(Location.X / P), 0, GridSize),
		FMath::Clamp(FMath::RoundToInt(Location.Y / P), 0, GridSize));
}

FVector AOCCityGenerator::GetSidewalkCorner(FIntPoint Block, int32 Corner) const
{
	const float P = GetPitch();
	const float Inset = RoadWidth * 0.5f + SidewalkWidth * 0.5f;
	const float MinX = Block.X * P + Inset;
	const float MaxX = (Block.X + 1) * P - Inset;
	const float MinY = Block.Y * P + Inset;
	const float MaxY = (Block.Y + 1) * P - Inset;
	const float Z = GetSidewalkTopZ() + 92.f;

	switch (Corner & 3)
	{
	case 0: return FVector(MinX, MinY, Z);
	case 1: return FVector(MaxX, MinY, Z);
	case 2: return FVector(MaxX, MaxY, Z);
	default: return FVector(MinX, MaxY, Z);
	}
}

void AOCCityGenerator::FindNearestSidewalkCorner(const FVector& Location, FIntPoint& OutBlock, int32& OutCorner) const
{
	const float P = GetPitch();
	OutBlock = FIntPoint(
		FMath::Clamp(FMath::FloorToInt(Location.X / P), 0, GridSize - 1),
		FMath::Clamp(FMath::FloorToInt(Location.Y / P), 0, GridSize - 1));

	float BestDist = TNumericLimits<float>::Max();
	OutCorner = 0;
	for (int32 Corner = 0; Corner < 4; ++Corner)
	{
		const float Dist = FVector::DistSquared2D(Location, GetSidewalkCorner(OutBlock, Corner));
		if (Dist < BestDist)
		{
			BestDist = Dist;
			OutCorner = Corner;
		}
	}
}

bool AOCCityGenerator::FindSidewalkSpawn(const FVector& Origin, float MinDist, float MaxDist, FIntPoint& OutBlock, int32& OutCorner) const
{
	const float P = GetPitch();
	const int32 Range = FMath::CeilToInt(MaxDist / P) + 1;
	const FIntPoint Center(FMath::FloorToInt(Origin.X / P), FMath::FloorToInt(Origin.Y / P));

	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const FIntPoint Block(Center.X + FMath::RandRange(-Range, Range), Center.Y + FMath::RandRange(-Range, Range));
		if (!IsValidBlock(Block))
		{
			continue;
		}
		const int32 Corner = FMath::RandRange(0, 3);
		const float Dist = FVector::Dist2D(Origin, GetSidewalkCorner(Block, Corner));
		if (Dist >= MinDist && Dist <= MaxDist)
		{
			OutBlock = Block;
			OutCorner = Corner;
			return true;
		}
	}
	return false;
}

bool AOCCityGenerator::FindLaneSpawn(const FVector& Origin, float MinDist, float MaxDist, FTransform& OutTransform, FIntPoint& OutNextIntersection, FIntPoint& OutDirection) const
{
	const FIntPoint Center = GetNearestIntersection(Origin);
	const int32 Range = FMath::CeilToInt(MaxDist / GetPitch()) + 1;
	const FIntPoint Directions[] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

	for (int32 Attempt = 0; Attempt < 16; ++Attempt)
	{
		const FIntPoint From(Center.X + FMath::RandRange(-Range, Range), Center.Y + FMath::RandRange(-Range, Range));
		const FIntPoint Dir = Directions[FMath::RandRange(0, 3)];
		const FIntPoint To = From + Dir;
		if (!IsValidIntersection(From) || !IsValidIntersection(To))
		{
			continue;
		}

		const FVector Right(-Dir.Y, Dir.X, 0.f);
		FVector Location = FMath::Lerp(GetIntersectionLocation(From), GetIntersectionLocation(To), FMath::FRandRange(0.25f, 0.75f)) + Right * GetLaneOffset();
		Location.Z = AOCVehicle::GetRestHeight();
		const float Dist = FVector::Dist2D(Origin, Location);
		if (Dist < MinDist || Dist > MaxDist)
		{
			continue;
		}

		OutTransform = FTransform(FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Dir.Y), static_cast<float>(Dir.X))), 0.f), Location);
		OutNextIntersection = To;
		OutDirection = Dir;
		return true;
	}
	return false;
}

bool AOCCityGenerator::IsPathClear(const FVector& A, const FVector& B) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OCCityNav), false);
	return !GetWorld()->LineTraceTestByObjectType(FVector(A.X, A.Y, 80.f), FVector(B.X, B.Y, 80.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

FVector AOCCityGenerator::GetNavWaypoint(const FVector& From, const FVector& To) const
{
	if (IsPathClear(From, To))
	{
		return To;
	}

	const FIntPoint Here = GetNearestIntersection(From);
	const FIntPoint Goal = GetNearestIntersection(To);
	const FVector HereLocation = GetIntersectionLocation(Here);
	if (FVector::Dist2D(From, HereLocation) > 450.f)
	{
		return HereLocation;
	}
	if (Here == Goal)
	{
		return To;
	}

	FIntPoint Step = Here;
	const FIntPoint Delta = Goal - Here;
	if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
	{
		Step.X += FMath::Sign(Delta.X);
	}
	else
	{
		Step.Y += FMath::Sign(Delta.Y);
	}
	return GetIntersectionLocation(Step);
}

FTransform AOCCityGenerator::GetPlayerSpawnTransform() const
{
	return FTransform(FRotator(0.f, 45.f, 0.f), GetSidewalkCorner(SpawnBlock, 0));
}
