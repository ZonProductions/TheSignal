// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveDepositLibrary.h"
#include "ZP_ObjectiveSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"

namespace
{
	// Find the Item_ (object) and Amount (int) fields inside a Moonville FFItemSlot struct.
	void FindSlotFields(FStructProperty* Inner, FObjectProperty*& OutItem, FIntProperty*& OutAmount)
	{
		OutItem = nullptr; OutAmount = nullptr;
		if (!Inner) return;
		for (TFieldIterator<FProperty> It(Inner->Struct); It; ++It)
		{
			if (!OutItem && It->GetName().Contains(TEXT("Item_")))   OutItem = CastField<FObjectProperty>(*It);
			if (!OutAmount && It->GetName().Contains(TEXT("Amount"))) OutAmount = CastField<FIntProperty>(*It);
		}
	}

	// Sum the stack Amounts of TargetDA across the inventory component's ItemSlots array.
	int32 CountInItemSlots(UActorComponent* Inv, UObject* TargetDA)
	{
		if (!Inv || !TargetDA) return 0;
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(Inv->GetClass()->FindPropertyByName(FName("ItemSlots")));
		if (!ArrayProp) return 0;
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Inv));
		FObjectProperty* ItemProp = nullptr; FIntProperty* AmountProp = nullptr;
		FindSlotFields(CastField<FStructProperty>(ArrayProp->Inner), ItemProp, AmountProp);
		if (!ItemProp) return 0;

		int32 Total = 0;
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			void* Elem = Helper.GetRawPtr(i);
			UObject* SlotItem = ItemProp->GetObjectPropertyValue(ItemProp->ContainerPtrToValuePtr<void>(Elem));
			if (SlotItem == TargetDA)
			{
				const int32 Amt = AmountProp ? AmountProp->GetPropertyValue(AmountProp->ContainerPtrToValuePtr<void>(Elem)) : 1;
				Total += FMath::Max(Amt, 1);
			}
		}
		return Total;
	}

	// --- Objective container status light (runtime-created, NOT an SCS component — never touches
	//     the Moonville BP's root) + interaction lock helpers. ---
	//
	// Per-instance knobs are READ FROM THE CONTAINER BP via reflection (set in the level Details panel):
	//   StatusLightIntensity        (float)        — brightness, candelas.            Default 300.
	//   StatusLightColorIncomplete  (LinearColor)  — beacon color while NEEDS items.  Default white.
	//   bStatusLightOnComplete      (bool)         — keep a light AFTER completion?    Default false (none).
	//   StatusLightColorComplete    (LinearColor)  — color once DONE (keypad green…).  Default green.
	// Defaults below are used when a knob var is absent, so the light still works before the vars exist.

	const TCHAR* GStatusLightName = TEXT("ZP_ObjectiveStatusLight");
	constexpr float AZP_GDefaultLightIntensity = 300.f;  // candelas (was a fixed 150 — bumped; now tunable)
	constexpr float AZP_GStatusLightRadius     = 300.f;  // attenuation radius (UU)
	constexpr float AZP_GStatusLightHeight     = 80.f;   // Z offset above the container root

	float ReadFloatProp(const AActor* A, const TCHAR* Name, float Def)
	{
		FProperty* P = A ? A->GetClass()->FindPropertyByName(FName(Name)) : nullptr;
		if (const FDoubleProperty* DP = CastField<FDoubleProperty>(P)) return (float)DP->GetPropertyValue_InContainer(A); // BP "float" is double
		if (const FFloatProperty* FP = CastField<FFloatProperty>(P)) return FP->GetPropertyValue_InContainer(A);
		return Def;
	}
	bool ReadBoolProp(const AActor* A, const TCHAR* Name, bool Def)
	{
		if (const FBoolProperty* P = A ? CastField<FBoolProperty>(A->GetClass()->FindPropertyByName(FName(Name))) : nullptr)
			return P->GetPropertyValue_InContainer(A);
		return Def;
	}
	FLinearColor ReadLinearColorProp(const AActor* A, const TCHAR* Name, FLinearColor Def)
	{
		if (const FStructProperty* P = A ? CastField<FStructProperty>(A->GetClass()->FindPropertyByName(FName(Name))) : nullptr)
		{
			if (P->Struct == TBaseStructure<FLinearColor>::Get())
				return *P->ContainerPtrToValuePtr<FLinearColor>(A);
		}
		return Def;
	}

	// A freshly-added LinearColor BP var defaults to (0,0,0,0). Treat an unset/black color as "use the
	// default" so the beacon is never invisible-black — the dev still overrides it with any real color.
	FLinearColor ColorOrDefault(FLinearColor C, FLinearColor Def)
	{
		return (C.R <= 0.f && C.G <= 0.f && C.B <= 0.f) ? Def : C;
	}

	UPointLightComponent* FindStatusLight(AActor* Container)
	{
		if (!Container) return nullptr;
		TArray<UPointLightComponent*> Lights;
		Container->GetComponents(Lights);
		for (UPointLightComponent* L : Lights)
		{
			if (L && L->GetName() == GStatusLightName)
			{
				return L;
			}
		}
		return nullptr;
	}

	void RemoveStatusLight(AActor* Container)
	{
		if (UPointLightComponent* L = FindStatusLight(Container))
		{
			L->DestroyComponent();
		}
	}

	// Create-or-update the named status light at the given color/intensity. Intensity <= 0 removes it
	// (lets a dev turn the light off entirely by setting StatusLightIntensity to 0 on a given container).
	void SetStatusLight(AActor* Container, FLinearColor Color, float Intensity)
	{
		if (!Container) return;
		if (Intensity <= 0.f) { RemoveStatusLight(Container); return; }
		USceneComponent* Root = Container->GetRootComponent();
		if (!Root) return;

		UPointLightComponent* Light = FindStatusLight(Container);
		if (!Light)
		{
			Light = NewObject<UPointLightComponent>(Container, UPointLightComponent::StaticClass(), FName(GStatusLightName));
			if (!Light) return;
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetAttenuationRadius(AZP_GStatusLightRadius);
			Light->SetCastShadows(false);
			Light->RegisterComponent();
			Light->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			Light->SetRelativeLocation(FVector(0.f, 0.f, AZP_GStatusLightHeight));
		}
		Light->SetLightColor(Color);
		Light->SetIntensity(Intensity);
	}

	// Beacon while the container still NEEDS items: configured incomplete color at configured intensity.
	void ShowIncompleteLight(AActor* Container)
	{
		SetStatusLight(Container,
			ColorOrDefault(ReadLinearColorProp(Container, TEXT("StatusLightColorIncomplete"), FLinearColor::White), FLinearColor::White),
			ReadFloatProp(Container, TEXT("StatusLightIntensity"), AZP_GDefaultLightIntensity));
	}

	// Once DONE: by default remove the light (no color); or, if bStatusLightOnComplete, recolor it to the
	// "complete" color so a physical device stays lit in its done state (keypad green, alarm off→green, etc.).
	void ApplyCompleteLight(AActor* Container)
	{
		if (ReadBoolProp(Container, TEXT("bStatusLightOnComplete"), false))
		{
			SetStatusLight(Container,
				ColorOrDefault(ReadLinearColorProp(Container, TEXT("StatusLightColorComplete"), FLinearColor::Green), FLinearColor::Green),
				ReadFloatProp(Container, TEXT("StatusLightIntensity"), AZP_GDefaultLightIntensity));
		}
		else
		{
			RemoveStatusLight(Container);
		}
	}

	// Permanently stop the container from being interacted with / reopened: kill every interaction
	// sphere (same mechanism AZP_GraceCharacter::DisableLockerInteraction uses for empty lockers).
	// The visual mesh keeps its own collision; only the overlap/trace interaction volumes go dark.
	void LockContainerInteraction(AActor* Container)
	{
		if (!Container) return;
		TArray<USphereComponent*> Spheres;
		Container->GetComponents(Spheres);
		for (USphereComponent* S : Spheres)
		{
			if (!S) continue;
			S->SetGenerateOverlapEvents(false);
			S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	UZP_ObjectiveSubsystem* GetObjectiveSubsystem(AActor* Container)
	{
		UWorld* World = Container ? Container->GetWorld() : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
	}

	bool IsObjectiveFlagSet(AActor* Container, FName Flag)
	{
		if (Flag.IsNone()) return false;
		UZP_ObjectiveSubsystem* Obj = GetObjectiveSubsystem(Container);
		return Obj ? Obj->HasFlag(Flag) : false;
	}

	// Read an FText BP var off the container (the per-instance authored title/body). Empty if the
	// var doesn't exist — i.e. this is a plain loot container, not an objective container.
	FText ReadTextProp(const AActor* A, const TCHAR* Name)
	{
		if (const FTextProperty* P = A ? CastField<FTextProperty>(A->GetClass()->FindPropertyByName(FName(Name))) : nullptr)
			return P->GetPropertyValue_InContainer(A);
		return FText::GetEmpty();
	}

	// Find a named child widget on the menu by tree lookup — works for ANY widget (variable or not), so it
	// also resolves the pack's "Item Container" header label. Returns null if the menu has no such widget.
	UWidget* FindNamedWidget(UUserWidget* Menu, const TCHAR* Name)
	{
		return Menu ? Menu->GetWidgetFromName(FName(Name)) : nullptr;
	}
}

UActorComponent* UZP_ObjectiveDepositLibrary::FindContainerInventory(AActor* Container)
{
	if (!Container) return nullptr;
	// The deposit grid is the inventory component that owns an ItemSlots array (same probe FilterLockerAmmo uses).
	for (UActorComponent* C : Container->GetComponents())
	{
		if (C && C->GetClass()->FindPropertyByName(FName("ItemSlots")))
		{
			return C;
		}
	}
	return nullptr;
}

int32 UZP_ObjectiveDepositLibrary::RequiredCellCount(const TArray<FZP_RequiredItem>& RequiredItems)
{
	int32 N = 0;
	for (const FZP_RequiredItem& R : RequiredItems)
	{
		N += FMath::Max(R.Count, 1);
	}
	return N;
}

void UZP_ObjectiveDepositLibrary::SetupDepositGrid(UActorComponent* ContainerInventory, const TArray<FZP_RequiredItem>& RequiredItems)
{
	if (!ContainerInventory) return;
	const int32 N = FMath::Max(RequiredCellCount(RequiredItems), 1);

	// Base grid size from the config asset (default to 1x1 if unreadable).
	FVector2D Base(1.f, 1.f);
	if (FObjectProperty* CfgProp = CastField<FObjectProperty>(ContainerInventory->GetClass()->FindPropertyByName(FName("InventoryConfig"))))
	{
		if (UObject* Cfg = CfgProp->GetObjectPropertyValue(CfgProp->ContainerPtrToValuePtr<void>(ContainerInventory)))
		{
			if (FStructProperty* SizeProp = CastField<FStructProperty>(Cfg->GetClass()->FindPropertyByName(FName("InventorySize"))))
			{
				Base = *SizeProp->ContainerPtrToValuePtr<FVector2D>(Cfg);
			}
		}
	}

	// GetInventorySize() == InventoryConfig.InventorySize + InventorySizeExpansion. Target one row of N.
	if (FStructProperty* ExpProp = CastField<FStructProperty>(ContainerInventory->GetClass()->FindPropertyByName(FName("InventorySizeExpansion"))))
	{
		FVector2D* Exp = ExpProp->ContainerPtrToValuePtr<FVector2D>(ContainerInventory);
		*Exp = FVector2D(static_cast<float>(N), 1.f) - Base;
	}
}

void UZP_ObjectiveDepositLibrary::SetupDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems)
{
	if (UActorComponent* Inv = FindContainerInventory(Container))
	{
		SetupDepositGrid(Inv, RequiredItems);
	}
}

bool UZP_ObjectiveDepositLibrary::ValidateDeposit(UActorComponent* ContainerInventory, const TArray<FZP_RequiredItem>& RequiredItems)
{
	if (!ContainerInventory || RequiredItems.Num() == 0) return false;
	for (const FZP_RequiredItem& R : RequiredItems)
	{
		UObject* DA = R.Item.LoadSynchronous();
		if (!DA) return false;
		if (CountInItemSlots(ContainerInventory, DA) < FMath::Max(R.Count, 1))
		{
			return false;
		}
	}
	return true;
}

bool UZP_ObjectiveDepositLibrary::SubmitDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, bool bConsume, FName ObjectiveFlag)
{
	UActorComponent* Inv = FindContainerInventory(Container);
	if (!Inv || !ValidateDeposit(Inv, RequiredItems))
	{
		return false;
	}

	if (bConsume)
	{
		if (UFunction* RemoveFunc = Inv->FindFunction(FName("RemoveItemByDataAsset")))
		{
			for (const FZP_RequiredItem& R : RequiredItems)
			{
				UObject* DA = R.Item.LoadSynchronous();
				if (!DA) continue;
				struct { UObject* AZP_ItemDataAsset; int32 AmountToRemove; } Params;
				Params.AZP_ItemDataAsset = DA;
				Params.AmountToRemove = FMath::Max(R.Count, 1); // exact total — Moonville rejects over-removal
				Inv->ProcessEvent(RemoveFunc, &Params);
			}
		}
	}

	if (!ObjectiveFlag.IsNone() && Container)
	{
		UWorld* World = Container->GetWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr)
		{
			Obj->SetFlag(ObjectiveFlag);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveDeposit: SubmitDeposit OK on %s (flag=%s, consumed=%d)"),
		*GetNameSafe(Container), *ObjectiveFlag.ToString(), bConsume ? 1 : 0);
	return true;
}

void UZP_ObjectiveDepositLibrary::SetupObjectiveContainer(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, FName ObjectiveFlag)
{
	if (!Container) return;

	// Size the deposit grid to exactly the items required (one row of N cells).
	if (UActorComponent* Inv = FindContainerInventory(Container))
	{
		SetupDepositGrid(Inv, RequiredItems);
	}

	// Fail loud: no flag = completion can never persist across save/load (and can't drive objectives).
	if (ObjectiveFlag.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveDeposit: %s has NO ObjectiveFlag — completion will NOT persist or advance any objective. Set ObjectiveFlag in the container's Details."),
			*GetNameSafe(Container));
	}

	// Restored-from-save: this objective was already completed — load it already locked, with the
	// configured "complete" light state (none by default; the complete color if bStatusLightOnComplete).
	if (IsObjectiveFlagSet(Container, ObjectiveFlag))
	{
		ApplyCompleteLight(Container);
		LockContainerInteraction(Container);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveDeposit: %s restored COMPLETE (flag=%s) — locked"),
			*GetNameSafe(Container), *ObjectiveFlag.ToString());
		return;
	}

	// Still needs items: show the "needs items" beacon (configured incomplete color + intensity).
	ShowIncompleteLight(Container);
}

bool UZP_ObjectiveDepositLibrary::ProcessObjectiveDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, FName ObjectiveFlag)
{
	if (!Container) return false;

	// Already done (flag persisted / completed earlier this session) — stay locked, no-op.
	if (IsObjectiveFlagSet(Container, ObjectiveFlag))
	{
		return false;
	}

	// Complete ONLY when every required item is present in at least its Count — never on a partial deposit.
	UActorComponent* Inv = FindContainerInventory(Container);
	if (!Inv || !ValidateDeposit(Inv, RequiredItems))
	{
		// The player opened and closed the box without completing it — they have SEEN it. Set
		// "<ObjectiveFlag>_TOUCHED" so hidden sub-objectives can reveal off the first touch
		// (e.g. FUSE_BOX_TOUCHED reveals "Find 3 fuses" in the tracker).
		if (ObjectiveFlag != NAME_None)
		{
			if (UZP_ObjectiveSubsystem* Obj = GetObjectiveSubsystem(Container))
			{
				const FName TouchedFlag(*(ObjectiveFlag.ToString() + TEXT("_TOUCHED")));
				if (!Obj->HasFlag(TouchedFlag))
				{
					Obj->SetFlag(TouchedFlag);
				}
			}
		}
		return false; // items still missing — leave it open/interactable, beacon stays lit
	}

	// --- Objective complete ---
	if (UZP_ObjectiveSubsystem* Obj = GetObjectiveSubsystem(Container))
	{
		if (!ObjectiveFlag.IsNone())
		{
			Obj->SetFlag(ObjectiveFlag); // persists; advances objectives
		}
	}

	// Apply the completion light state (off by default; or the "complete" color for physical devices) +
	// permanently lock interaction. Deposited items stay in the locked container — no consume.
	ApplyCompleteLight(Container);
	LockContainerInteraction(Container);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveDeposit: %s COMPLETE (flag=%s) — beacon off, interaction locked"),
		*GetNameSafe(Container), *ObjectiveFlag.ToString());
	return true;
}

bool UZP_ObjectiveDepositLibrary::TryAutoCompleteObjectiveContainer(AActor* Container)
{
	if (!Container) return false;
	UClass* Cls = Container->GetClass();

	// RequiredItems is a BP variable of the actual C++ struct type (FZP_RequiredItem), so the array
	// memory is layout-compatible with TArray<FZP_RequiredItem> — read it directly via reflection.
	// Moonville-child containers (BP_ObjectiveContainer) declare it as a BP variable under the
	// ORIGINAL name; the C++ sibling (AZP_ObjectiveContainer) carries the AZP_-prefixed knob — try both.
	FArrayProperty* RIProp = CastField<FArrayProperty>(Cls->FindPropertyByName(FName("RequiredItems")));
	if (!RIProp)
	{
		RIProp = CastField<FArrayProperty>(Cls->FindPropertyByName(FName("AZP_RequiredItems")));
	}
	if (!RIProp)
	{
		return false;
	}
	const TArray<FZP_RequiredItem>& RequiredItems =
		*RIProp->ContainerPtrToValuePtr<TArray<FZP_RequiredItem>>(Container);

	FName ObjectiveFlag = NAME_None;
	if (FNameProperty* FlagProp = CastField<FNameProperty>(Cls->FindPropertyByName(FName("ObjectiveFlag"))))
	{
		ObjectiveFlag = FlagProp->GetPropertyValue_InContainer(Container);
	}

	return ProcessObjectiveDeposit(Container, RequiredItems, ObjectiveFlag);
}

void UZP_ObjectiveDepositLibrary::ApplyObjectiveContainerMenu(UUserWidget* Menu, AActor* Container)
{
	if (!Menu) return;

	// "CommonTextBlock" = the pack's "Item Container" header above the container/transfer slots.
	UTextBlock* HeaderTB = Cast<UTextBlock>(FindNamedWidget(Menu, TEXT("CommonTextBlock")));
	UTextBlock* BodyTB   = Cast<UTextBlock>(FindNamedWidget(Menu, TEXT("ObjectiveBodyText")));

	// Per-instance authored text off the OPEN container. A plain loot container has no such vars, so both
	// come back empty: the header keeps its design-time "Item Container" and the description stays hidden.
	const FText Title = ReadTextProp(Container, TEXT("ContainerTitle"));
	const FText Body  = ReadTextProp(Container, TEXT("ContainerDescription"));

	// The container title REPLACES the "Item Container" header label (only when authored).
	if (HeaderTB && !Title.IsEmpty())
	{
		HeaderTB->SetText(Title);
	}

	// The description sits directly below the transfer slots. SelfHitTestInvisible so it never eats a click
	// meant for an item slot; collapsed (no layout footprint) when there's no description.
	if (BodyTB)
	{
		BodyTB->SetText(Body);
		BodyTB->SetVisibility(!Body.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
