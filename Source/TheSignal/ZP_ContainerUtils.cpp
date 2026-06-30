// Copyright The Signal. All Rights Reserved.

#include "ZP_ContainerUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"

void UZP_ContainerUtils::FitBoxToMeshBounds(UStaticMeshComponent* Mesh, UBoxComponent* Box, float Padding)
{
	if (!Mesh || !Box)
	{
		return;
	}

	UStaticMesh* SM = Mesh->GetStaticMesh();
	if (!SM)
	{
		// No mesh chosen yet — leave the box untouched.
		return;
	}

	// Mesh-local bounding box (unscaled). Box is a child of the mesh, so it inherits the
	// mesh's world scale: setting the unscaled extent/center here yields the correct world size.
	const FBox Local = SM->GetBoundingBox();
	Box->SetBoxExtent(Local.GetExtent() + FVector(Padding), true);
	Box->SetRelativeLocation(Local.GetCenter());
}
