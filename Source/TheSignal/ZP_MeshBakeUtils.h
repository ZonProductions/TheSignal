// Copyright The Signal. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZP_MeshBakeUtils.generated.h"

/**
 * Editor-only utility: bake the CURRENT POSE of one or more actors'
 * skeletal/static mesh components into a single UStaticMesh asset.
 *
 * Purpose:        C++ equivalent of the editor's "Convert to Static Mesh"
 *                 (FMeshMergeUtilities::MergeComponentsToStaticMesh). UE5.7
 *                 Python exposes NO skeletal->static bake and NO skinned-vertex
 *                 extraction, so this is the only programmatic path.
 * Owned by:       none (editor tooling, no runtime owner subsystem).
 * BP/Python entry:unreal.ZP_MeshBakeUtils.bake_actors_to_static_mesh(actors, pkg, save)
 * Dependencies:   MeshMergeUtilities (editor developer module, bBuildEditor only).
 *
 * Run it while the source is POSED (e.g. on the live PIE crawler, whose legs
 * are curled by ABP_Tentacle) so the bake captures that exact pose. The merge
 * reads each component's current evaluated pose — ref pose if unposed.
 */
UCLASS()
class UZP_MeshBakeUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bakes every valid skeletal/static mesh component on the given actors into
	 * one UStaticMesh at PackagePath (e.g. "/Game/Enemies/Crawler/SM_Crawler_Ref").
	 * Uses the actors' world (pass PIE-world actors to bake their live pose).
	 *
	 * @param Actors       Source actors (parent + attached parts). Components are
	 *                     gathered from each; child actors are NOT auto-traversed,
	 *                     so pass attached part actors explicitly.
	 * @param PackagePath  Long package path for the new static mesh asset.
	 * @param bSaveAsset   If true, save the package to disk immediately. Leave
	 *                     false when called during PIE (save after stopping PIE).
	 * @return The created UStaticMesh, or nullptr on failure (see log).
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static UStaticMesh* BakeActorsToStaticMesh(const TArray<AActor*>& Actors, const FString& PackagePath, bool bSaveAsset = false);
};

#endif
