#include "OCAssets.h"
#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UStaticMesh* OCAssets::Cube()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UStaticMesh* OCAssets::Cylinder()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

UStaticMesh* OCAssets::Sphere()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
}

UStaticMesh* OCAssets::Cone()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
}

UMaterialInterface* OCAssets::BaseMaterial()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

UMaterialInstanceDynamic* OCAssets::Paint(UMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!Mesh)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial(), Mesh);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		Mesh->SetMaterial(0, MID);
	}
	return MID;
}
