// Copyright The Signal. All Rights Reserved.

#include "ZP_BloodFXComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

// All values resolved for one composite spawn (instance overrides OR class defaults).
struct UZP_BloodFXComponent::FBloodSpawnParams
{
	UNiagaraSystem* HitSystem = nullptr; // intensity-selected melee OR ranged system
	UNiagaraSystem* Bleed = nullptr;
	UMaterialInterface* DecalMat = nullptr;
	FLinearColor Blood = FLinearColor::White;
	FLinearColor Smoke = FLinearColor::White;
	FLinearColor Decal = FLinearColor::White;
	float Scale = 1.f;
	float LifeMult = 1.f;
	float BleedTime = 1.5f;
	int32 WallSplats = 2;
	int32 FloorSplats = 2;
	float SizeMin = 26.f;
	float SizeMax = 58.f;
	float DecalLife = 0.f;
	float WallDist = 420.f;
	bool bFloorPool = true;
	float PoolDelaySec = 1.2f;
	float PoolMin = 28.f;
	float PoolMax = 44.f;
	bool bResidual = true;
	int32 ResidualN = 2;
	float ResidualMin = 10.f;
	float ResidualMax = 22.f;
	FName ColorP, SmokeP, ScaleP, LifeP, DecalColorP;
};

namespace
{
	// Default pack asset paths — single source of truth for lazy loads, CDO fallback, and prewarm.
	// Dev-picked off the pack's Systems Overview map (verified in PIE 2026-07-02):
	//   melee  = "Splash with Burst + Hit"  -> P_SplashWithBurst_Hit_01/02/03 (correct residual blood)
	//   ranged = "Splash + Hit + Metal"     -> P_Splash_HitWithMetal_01/02/03 (bullet impact/ricochet)
	const TCHAR* MeleeBloodPaths[3] = {
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_01.P_SplashWithBurst_Hit_01"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_02.P_SplashWithBurst_Hit_02"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_03.P_SplashWithBurst_Hit_03"),
	};
	const TCHAR* RangedBloodPaths[3] = {
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_01.P_Splash_HitWithMetal_01"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_02.P_Splash_HitWithMetal_02"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_03.P_Splash_HitWithMetal_03"),
	};
	const TCHAR* BloodDecalPath = TEXT("/Game/Blood_VFX_Pack/Materials/MI_BloodDecal.MI_BloodDecal");
}

UZP_BloodFXComponent::UZP_BloodFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UNiagaraSystem* UZP_BloodFXComponent::LoadSystem(const TCHAR* Path)
{
	return LoadObject<UNiagaraSystem>(nullptr, Path);
}

void UZP_BloodFXComponent::EnsureAssets()
{
	// Lazy loads — never ConstructorHelpers /Game pack assets on a component (editor-load crash lesson).
	// NOTE: BleedSystem is deliberately NOT defaulted — the pack's bleeding systems read as a gag
	// (dev-rejected). The quiet aftermath is the delayed floor pool instead.
	if (MeleeBloodSystems.Num() == 0)
	{
		for (const TCHAR* Path : MeleeBloodPaths)
		{
			MeleeBloodSystems.Add(LoadSystem(Path));
		}
	}
	if (RangedBloodSystems.Num() == 0)
	{
		for (const TCHAR* Path : RangedBloodPaths)
		{
			RangedBloodSystems.Add(LoadSystem(Path));
		}
	}
	if (!DecalMaterial)
	{
		DecalMaterial = LoadObject<UMaterialInterface>(nullptr, BloodDecalPath);
	}
}

void UZP_BloodFXComponent::BeginPlay()
{
	Super::BeginPlay();

	// Pay ALL costs at level load, not on the first swing: load the assets now and prewarm their
	// shaders/PSOs once per world (first-hit checkerboard + lag spike fix).
	EnsureAssets();
	PrewarmWorldOnce(GetWorld());

	// World-space decals can't deform with animation — anything projected onto the body smears as
	// it moves ("glitchy purple spots"). The body opts out; walls and floors take the splatter.
	if (bDisableBodyDecals && GetOwner())
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(GetOwner());
		for (USkeletalMeshComponent* M : Meshes)
		{
			if (M) { M->SetReceivesDecals(false); }
		}
	}
}

void UZP_BloodFXComponent::PrewarmWorldOnce(UWorld* World)
{
	if (!World || !World->IsGameWorld()) { return; }
	static TWeakObjectPtr<UWorld> PrewarmedWorld;
	if (PrewarmedWorld.Get() == World) { return; }
	PrewarmedWorld = World;

	// Fire each system + one decal far below the map at tiny scale: Niagara render resources,
	// materials, and PSOs all compile during load. Everything auto-destroys. All six variants are
	// prewarmed so any per-enemy BloodIntensity choice is hitch-free.
	const FVector Below(0.f, 0.f, -100000.f);
	auto Prewarm = [&](const TCHAR* Path)
	{
		if (UNiagaraSystem* Sys = LoadObject<UNiagaraSystem>(nullptr, Path))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Sys, Below, FRotator::ZeroRotator,
				FVector(0.05f), /*bAutoDestroy=*/true, /*bAutoActivate=*/true, ENCPoolMethod::None);
		}
	};
	for (const TCHAR* Path : MeleeBloodPaths) { Prewarm(Path); }
	for (const TCHAR* Path : RangedBloodPaths) { Prewarm(Path); }
	if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, BloodDecalPath))
	{
		if (UDecalComponent* D = UGameplayStatics::SpawnDecalAtLocation(
			World, Mat, FVector(8.f, 8.f, 8.f), Below, FRotator::ZeroRotator, /*LifeSpan=*/3.f))
		{
			if (UMaterialInstanceDynamic* MID = D->CreateDynamicMaterialInstance())
			{
				MID->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor::Black);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[BloodFX] prewarmed blood systems/decal shaders on %s"), *World->GetName());
}

void UZP_BloodFXComponent::PlayHitBlood(const FVector& Location, const FVector& Direction, bool bMeleeHit,
	FVector SurfaceNormal, AActor* ExtraIgnoreActor)
{
	EnsureAssets();
	FBloodSpawnParams P;
	const TArray<TObjectPtr<UNiagaraSystem>>& Set = bMeleeHit ? MeleeBloodSystems : RangedBloodSystems;
	const int32 Idx = FMath::Clamp(BloodIntensity - 1, 0, Set.Num() - 1);
	P.HitSystem = Set.IsValidIndex(Idx) ? Set[Idx].Get() : nullptr;
	P.Bleed = BleedSystem; P.DecalMat = DecalMaterial;
	P.Blood = BloodColor; P.Smoke = SmokeColor; P.Decal = DecalColor;
	P.Scale = BloodScale; P.LifeMult = LifeTimeMult; P.BleedTime = BleedDuration;
	P.WallSplats = NumWallSplats; P.FloorSplats = NumFloorSplats;
	P.SizeMin = DecalSizeMin; P.SizeMax = DecalSizeMax; P.DecalLife = DecalLifetime; P.WallDist = WallTraceDistance;
	P.bFloorPool = bDelayedFloorPool; P.PoolDelaySec = PoolDelay; P.PoolMin = PoolSizeMin; P.PoolMax = PoolSizeMax;
	P.bResidual = bBodyResidual; P.ResidualN = NumBodyResiduals; P.ResidualMin = BodyResidualSizeMin; P.ResidualMax = BodyResidualSizeMax;
	P.ColorP = ColorParam; P.SmokeP = SmokeColorParam; P.ScaleP = ScaleParam; P.LifeP = LifeTimeParam; P.DecalColorP = DecalColorParam;
	SpawnComposite(GetWorld(), GetOwner(), Location, Direction, bMeleeHit, SurfaceNormal, P, ExtraIgnoreActor);

	// Clinging body wound — the pack's human-painted decal art projected in pre-skinned space,
	// the tech that survives animation where world decals never could. Instance path only.
	if (bBodyWounds)
	{
		ApplyBodyWound(GetOwner(), Location, SurfaceNormal);
	}
}

void UZP_BloodFXComponent::ApplyBodyWound(AActor* HitActor, const FVector& WorldLocation, const FVector& SurfaceNormal)
{
	if (!HitActor || MaxBodyWounds <= 0) { return; }
	USkeletalMeshComponent* Mesh = HitActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh || !Mesh->GetSkeletalMeshAsset()) { return; }

	const FName Bone = Mesh->FindClosestBone(WorldLocation);
	const int32 BoneIdx = Mesh->GetBoneIndex(Bone);
	if (BoneIdx == INDEX_NONE) { return; }

	// World hit -> CURRENT bone local -> reference-pose (pre-skinned, mesh-local) space. The skin
	// shader compares this against the Pre-Skinned Local Position node, which is evaluated in the
	// same space — so the wound is welded to the skin through any pose.
	const FTransform BoneWorld = Mesh->GetBoneTransform(BoneIdx);
	const FVector BoneLocal = BoneWorld.InverseTransformPosition(WorldLocation);
	const FReferenceSkeleton& RefSkel = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
	FTransform RefComp = FTransform::Identity;
	for (int32 b = BoneIdx; b != INDEX_NONE; b = RefSkel.GetParentIndex(b))
	{
		RefComp = RefComp * RefSkel.GetRefBonePose()[b];
	}
	const FVector PreSkinned = RefComp.TransformPosition(BoneLocal);

	// MIDs wrap the creature's MI_ZP_*Skin (child of M_ZP_CreatureSkin) so params resolve.
	if (!bWoundMIDsCreated)
	{
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			Mesh->CreateAndSetMaterialInstanceDynamic(i);
		}
		bWoundMIDsCreated = true;
	}

	// Radius: world units -> mesh-local units (the shader space); mesh components here run scale
	// compensation (e.g. the Scytheer's 0.5), so divide it out.
	const float MeshScale = FMath::Max(Mesh->GetComponentTransform().GetScale3D().GetAbsMax(), KINDA_SMALL_NUMBER);
	const float LocalRadius = FMath::FRandRange(WoundRadiusMin, WoundRadiusMax) / MeshScale;

	// Projection frame for the pack's decal texture: tangent/bitangent perpendicular to the hit's
	// surface normal, random-rolled so repeated splats never align, carried into pre-skinned space
	// through the same bone chain as the position.
	FVector N = SurfaceNormal.IsNearlyZero() ? FVector::UpVector : SurfaceNormal.GetSafeNormal();
	FVector T = FVector::CrossProduct(N, FVector::UpVector);
	if (!T.Normalize())
	{
		T = FVector::CrossProduct(N, FVector::RightVector).GetSafeNormal();
	}
	FVector B = FVector::CrossProduct(N, T);
	const float Roll = FMath::FRandRange(0.f, 2.f * PI);
	T = T.RotateAngleAxisRad(Roll, N);
	B = B.RotateAngleAxisRad(Roll, N);
	auto ToPreSkinnedDir = [&BoneWorld, &RefComp](const FVector& Dir)
	{
		return RefComp.TransformVectorNoScale(BoneWorld.InverseTransformVectorNoScale(Dir)).GetSafeNormal();
	};
	const FVector TPre = ToPreSkinnedDir(T);
	const FVector BPre = ToPreSkinnedDir(B);

	const int32 Slot = (NextWoundSlot % FMath::Max(MaxBodyWounds, 1)) + 1;
	++NextWoundSlot;
	const FName LocParam(*FString::Printf(TEXT("WoundLoc_%d"), Slot));
	const FName TParam(*FString::Printf(TEXT("WoundT_%d"), Slot));
	const FName BParam(*FString::Printf(TEXT("WoundB_%d"), Slot));
	const FName RadParam(*FString::Printf(TEXT("WoundRadius_%d"), Slot));
	for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
	{
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i)))
		{
			MID->SetVectorParameterValue(FName(TEXT("WoundColor")), WoundColor);
			MID->SetVectorParameterValue(LocParam, FLinearColor(PreSkinned.X, PreSkinned.Y, PreSkinned.Z, 0.f));
			MID->SetVectorParameterValue(TParam, FLinearColor(TPre.X, TPre.Y, TPre.Z, 0.f));
			MID->SetVectorParameterValue(BParam, FLinearColor(BPre.X, BPre.Y, BPre.Z, 0.f));
			MID->SetScalarParameterValue(RadParam, LocalRadius);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[BloodFX] body wound slot %d bone=%s preSkinned=(%.1f, %.1f, %.1f) rLocal=%.1f"),
		Slot, *Bone.ToString(), PreSkinned.X, PreSkinned.Y, PreSkinned.Z, LocalRadius);
}

void UZP_BloodFXComponent::PlayHitBloodFor(AActor* HitActor, const FVector& Location, const FVector& Direction,
	bool bMeleeHit, FVector SurfaceNormal, AActor* ExtraIgnoreActor)
{
	if (!HitActor) { return; }

	if (UZP_BloodFXComponent* Comp = HitActor->FindComponentByClass<UZP_BloodFXComponent>())
	{
		Comp->PlayHitBlood(Location, Direction, bMeleeHit, SurfaceNormal, ExtraIgnoreActor);
		return;
	}

	// No component — class defaults so it still bleeds (infected purple).
	const UZP_BloodFXComponent* CDO = GetDefault<UZP_BloodFXComponent>();
	FBloodSpawnParams P;
	const int32 Idx = FMath::Clamp(CDO->BloodIntensity - 1, 0, 2);
	P.HitSystem = LoadSystem(bMeleeHit ? MeleeBloodPaths[Idx] : RangedBloodPaths[Idx]);
	P.Bleed = nullptr; // no default bleed (gag look, dev-rejected)
	P.DecalMat = LoadObject<UMaterialInterface>(nullptr, BloodDecalPath);
	P.Blood = CDO->BloodColor; P.Smoke = CDO->SmokeColor; P.Decal = CDO->DecalColor;
	P.Scale = CDO->BloodScale; P.LifeMult = CDO->LifeTimeMult; P.BleedTime = CDO->BleedDuration;
	P.WallSplats = CDO->NumWallSplats; P.FloorSplats = CDO->NumFloorSplats;
	P.SizeMin = CDO->DecalSizeMin; P.SizeMax = CDO->DecalSizeMax; P.DecalLife = CDO->DecalLifetime; P.WallDist = CDO->WallTraceDistance;
	P.bFloorPool = CDO->bDelayedFloorPool; P.PoolDelaySec = CDO->PoolDelay; P.PoolMin = CDO->PoolSizeMin; P.PoolMax = CDO->PoolSizeMax;
	P.bResidual = CDO->bBodyResidual; P.ResidualN = CDO->NumBodyResiduals; P.ResidualMin = CDO->BodyResidualSizeMin; P.ResidualMax = CDO->BodyResidualSizeMax;
	P.ColorP = CDO->ColorParam; P.SmokeP = CDO->SmokeColorParam; P.ScaleP = CDO->ScaleParam; P.LifeP = CDO->LifeTimeParam; P.DecalColorP = CDO->DecalColorParam;
	SpawnComposite(HitActor->GetWorld(), HitActor, Location, Direction, bMeleeHit, SurfaceNormal, P, ExtraIgnoreActor);
}

void UZP_BloodFXComponent::SpawnTintedSystem(UWorld* World, UNiagaraSystem* System, const FVector& Location,
	const FRotator& Rotation, const FBloodSpawnParams& P)
{
	if (!World || !System) { return; }
	UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, System, Location, Rotation, FVector(P.Scale),
		/*bAutoDestroy=*/true, /*bAutoActivate=*/true, ENCPoolMethod::None);
	if (!NC) { return; }
	NC->SetVariableLinearColor(P.ColorP, P.Blood);
	NC->SetVariableLinearColor(P.SmokeP, P.Smoke);
	NC->SetVariableFloat(P.ScaleP, P.Scale);
	NC->SetVariableFloat(P.LifeP, P.LifeMult);
}

void UZP_BloodFXComponent::StampDecalAt(UWorld* World, const FHitResult& Hit, const FBloodSpawnParams& P,
	float SizeMin, float SizeMax)
{
	const float Size = FMath::FRandRange(SizeMin, SizeMax);
	FRotator DecalRot = (-Hit.ImpactNormal).Rotation();
	DecalRot.Roll = FMath::FRandRange(0.f, 360.f); // break up repetition
	UDecalComponent* D = UGameplayStatics::SpawnDecalAtLocation(
		World, P.DecalMat, FVector(32.f, Size, Size), Hit.ImpactPoint, DecalRot, P.DecalLife);
	if (D)
	{
		D->SetFadeScreenSize(0.0005f); // don't vanish at distance
		if (UMaterialInstanceDynamic* MID = D->CreateDynamicMaterialInstance())
		{
			MID->SetVectorParameterValue(P.DecalColorP, P.Decal);
		}
	}
	else
	{
		// PROBE (dev report 2026-07-02: ground decals missing) — decal component failed to spawn.
		UE_LOG(LogTemp, Warning, TEXT("[BloodProbe] SpawnDecalAtLocation returned NULL (mat=%s at %.0f,%.0f,%.0f)"),
			P.DecalMat ? *P.DecalMat->GetName() : TEXT("null"),
			Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);
	}
}

void UZP_BloodFXComponent::SpawnComposite(UWorld* World, AActor* HitActor, const FVector& Location,
	const FVector& Direction, bool bMeleeHit, const FVector& SurfaceNormal, const FBloodSpawnParams& P,
	AActor* ExtraIgnoreActor)
{
	if (!World) { return; }
	const FVector Dir = Direction.GetSafeNormal();
	const FRotator SprayRot = Dir.Rotation();
	// Residual stains project INTO the skin along the wound's surface normal — flat against the
	// epidermis instead of a deep slab along the swing (the "unnatural top edge" the dev saw).
	const FVector ResidualDir = SurfaceNormal.IsNearlyZero() ? Dir : (-SurfaceNormal).GetSafeNormal();

	// 1. The dev-picked hit system (melee "Splash with Burst + Hit" / ranged "Splash + Hit + Metal"),
	//    at the enemy's BloodIntensity, aimed along the hit direction.
	SpawnTintedSystem(World, P.HitSystem, Location, SprayRot, P);

	// 2. GUARANTEED splatter — traced decals that always land and stay.
	SpawnSplatterDecals(World, HitActor, Location, Dir, P, ExtraIgnoreActor);

	// 2b. Residual ON the body, attached to the nearest bone — follows animation and the death
	//     fall (world-space stains just hang in the air once the body moves away).
	if (P.bResidual)
	{
		SpawnBodyResidual(HitActor, Location, ResidualDir, P);
	}

	// 3. Quiet aftermath on melee hits: a small pool forms under the enemy a beat later — blood
	//    had time to run down. (Attached spurt systems read as a gag; only used if BP-assigned.)
	//    Weak captures only — raw UObject pointers must not sit in a timer across GC.
	if (bMeleeHit && P.bFloorPool && HitActor && P.DecalMat)
	{
		TWeakObjectPtr<AActor> WeakActor = HitActor;
		TWeakObjectPtr<UMaterialInterface> WeakMat = P.DecalMat;
		const float PoolMin = P.PoolMin, PoolMax = P.PoolMax, Life = P.DecalLife;
		const FName TintParam = P.DecalColorP;
		const FLinearColor Tint = P.Decal;
		FTimerHandle Unused;
		World->GetTimerManager().SetTimer(Unused, FTimerDelegate::CreateLambda(
			[WeakActor, WeakMat, PoolMin, PoolMax, Life, TintParam, Tint]()
		{
			AActor* A = WeakActor.Get();
			UMaterialInterface* Mat = WeakMat.Get();
			UWorld* W = A ? A->GetWorld() : nullptr;
			if (!W || !Mat) { return; }
			FCollisionQueryParams Q(FName(TEXT("ZPBloodPool")), false);
			Q.AddIgnoredActor(A);
			const FVector Feet = A->GetActorLocation();
			FHitResult Hit;
			if (W->LineTraceSingleByChannel(Hit, Feet, Feet - FVector(0.f, 0.f, 300.f), ECC_WorldStatic, Q))
			{
				const float Size = FMath::FRandRange(PoolMin, PoolMax);
				FRotator DecalRot = (-Hit.ImpactNormal).Rotation();
				DecalRot.Roll = FMath::FRandRange(0.f, 360.f);
				if (UDecalComponent* D = UGameplayStatics::SpawnDecalAtLocation(
					W, Mat, FVector(32.f, Size, Size), Hit.ImpactPoint, DecalRot, Life))
				{
					D->SetFadeScreenSize(0.0005f);
					if (UMaterialInstanceDynamic* MID = D->CreateDynamicMaterialInstance())
					{
						MID->SetVectorParameterValue(TintParam, Tint);
					}
				}
			}
		}), FMath::Max(0.1f, P.PoolDelaySec), false);
	}

	if (bMeleeHit && P.Bleed && HitActor && HitActor->GetRootComponent())
	{
		UNiagaraComponent* BleedNC = UNiagaraFunctionLibrary::SpawnSystemAttached(
			P.Bleed, HitActor->GetRootComponent(), NAME_None,
			Location, SprayRot, EAttachLocation::KeepWorldPosition,
			/*bAutoDestroy=*/true, /*bAutoActivate=*/true, ENCPoolMethod::None);
		if (BleedNC)
		{
			BleedNC->SetVariableLinearColor(P.ColorP, P.Blood);
			BleedNC->SetVariableFloat(P.ScaleP, P.Scale);
			TWeakObjectPtr<UNiagaraComponent> WeakBleed = BleedNC;
			FTimerHandle Unused;
			World->GetTimerManager().SetTimer(Unused, FTimerDelegate::CreateLambda([WeakBleed]()
			{
				if (UNiagaraComponent* C = WeakBleed.Get()) { C->Deactivate(); } // autodestroys when done
			}), FMath::Max(0.2f, P.BleedTime), false);
		}
	}
}

void UZP_BloodFXComponent::SpawnBodyResidual(AActor* HitActor, const FVector& Location,
	const FVector& ProjectDir, const FBloodSpawnParams& P)
{
	if (!HitActor || !P.DecalMat || P.ResidualN <= 0) { return; }
	USkeletalMeshComponent* Mesh = HitActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh || !Mesh->GetSkeletalMeshAsset()) { return; }

	// Attach each stain to the bone nearest the wound: it rides that bone through animation and
	// the death fall. ProjectDir = INTO the skin along the wound's surface normal, and the
	// projection box is SHALLOW (depth 8) — a stain lying on the epidermis, not a slab cutting
	// through the torso with hard box edges.
	const FName Bone = Mesh->FindClosestBone(Location);
	if (Bone.IsNone()) { return; }

	for (int32 i = 0; i < P.ResidualN; ++i)
	{
		const float Size = FMath::FRandRange(P.ResidualMin, P.ResidualMax);
		const FVector Jitter(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f));
		FRotator StainRot = ProjectDir.Rotation();
		StainRot.Roll = FMath::FRandRange(0.f, 360.f);
		UDecalComponent* D = UGameplayStatics::SpawnDecalAttached(
			P.DecalMat, FVector(8.f, Size, Size), Mesh, Bone,
			Location + Jitter, StainRot, EAttachLocation::KeepWorldPosition, P.DecalLife);
		if (D)
		{
			D->SetFadeScreenSize(0.0005f);
			if (UMaterialInstanceDynamic* MID = D->CreateDynamicMaterialInstance())
			{
				MID->SetVectorParameterValue(P.DecalColorP, P.Decal);
			}
		}
	}
}

void UZP_BloodFXComponent::SpawnSplatterDecals(UWorld* World, AActor* HitActor, const FVector& Origin,
	const FVector& Direction, const FBloodSpawnParams& P, AActor* ExtraIgnoreActor)
{
	// PROBE (dev report 2026-07-02: ground decals missing/degraded) — every bail, trace, and
	// result logs so the failing stage is visible in the output log. Strip once diagnosed.
	if (!World || !P.DecalMat)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BloodProbe] SpawnSplatterDecals BAIL: world=%d decalMat=%d"),
			World != nullptr, P.DecalMat != nullptr);
		return;
	}

	FCollisionQueryParams Q(FName(TEXT("ZPBloodSplatter")), /*bTraceComplex=*/false);
	if (HitActor) { Q.AddIgnoredActor(HitActor); }
	if (ExtraIgnoreActor) { Q.AddIgnoredActor(ExtraIgnoreActor); } // e.g. the GRABBER during a bite

	// Wall/geometry splats: jittered rays along the spray — the through-spray hitting whatever is
	// behind the enemy. Slight upward bias reads as arterial arc.
	int32 WallHits = 0;
	int32 FloorHits = 0;
	for (int32 i = 0; i < P.WallSplats; ++i)
	{
		FVector JDir = Direction + FVector(FMath::FRandRange(-0.18f, 0.18f), FMath::FRandRange(-0.18f, 0.18f), FMath::FRandRange(-0.05f, 0.28f));
		JDir.Normalize();
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Origin, Origin + JDir * P.WallDist, ECC_WorldStatic, Q))
		{
			// Never stamp on a Pawn: enemy/player meshes don't receive decals, so a capsule hit
			// just parks an invisible decal component in mid-air that paints whatever decal-
			// receiving prop later moves through it (the world-space float DEAD END).
			if (Hit.GetActor() && Hit.GetActor()->IsA<APawn>()) { continue; }
			StampDecalAt(World, Hit, P, P.SizeMin, P.SizeMax);
			++WallHits;
		}
	}

	// Floor splats: drops land under and past the wound.
	for (int32 i = 0; i < P.FloorSplats; ++i)
	{
		const FVector Start = Origin
			+ Direction * FMath::FRandRange(30.f, 170.f)
			+ FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 30.f);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 350.f), ECC_WorldStatic, Q))
		{
			if (Hit.GetActor() && Hit.GetActor()->IsA<APawn>()) { continue; } // same no-Pawn rule
			StampDecalAt(World, Hit, P, P.SizeMin, P.SizeMax);
			++FloorHits;
			UE_LOG(LogTemp, Warning, TEXT("[BloodProbe] floor %d/%d HIT %s (%s) impactZ=%.0f startZ=%.0f"),
				i + 1, P.FloorSplats,
				Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("?"),
				Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("?"),
				Hit.ImpactPoint.Z, Start.Z);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BloodProbe] floor %d/%d MISS — nothing WorldStatic within 350uu below (%.0f, %.0f, %.0f)"),
				i + 1, P.FloorSplats, Start.X, Start.Y, Start.Z);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[BloodProbe] splatter done: wall %d/%d floor %d/%d origin=(%.0f,%.0f,%.0f) mat=%s"),
		WallHits, P.WallSplats, FloorHits, P.FloorSplats, Origin.X, Origin.Y, Origin.Z,
		*P.DecalMat->GetName());
}
