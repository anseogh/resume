#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;

// The prototype ships without art: everything is built from engine basic shapes,
// tinted through the "Color" parameter of BasicShapeMaterial.
namespace OCAssets
{
	OPENCITY_API UStaticMesh* Cube();
	OPENCITY_API UStaticMesh* Cylinder();
	OPENCITY_API UStaticMesh* Sphere();
	OPENCITY_API UStaticMesh* Cone();
	OPENCITY_API UMaterialInterface* BaseMaterial();

	OPENCITY_API UMaterialInstanceDynamic* Paint(UMeshComponent* Mesh, const FLinearColor& Color);
}
