// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveSubsystem.h"
#include "Engine/DataTable.h"
#include "ZP_SaveGame.h"
#include "ZP_GraceCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

void UZP_ObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadDefinitions();

	// Restore any previously-saved progress BEFORE auto-starting, so start-on-load
	// objectives that were already completed/active don't restart.
	if (bAutoPersist)
	{
		LoadObjectiveState();
	}

	for (const FName& Id : StartOnLoadIds)
	{
		StartObjective(Id); // no-op if already active/completed from the restore above
	}
}

bool UZP_ObjectiveSubsystem::LoadDefinitions()
{
	Definitions.Reset();
	StartOnLoadIds.Reset();

	UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/DT_Objectives.DT_Objectives"));
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveSubsystem: DT_Objectives not found at /Game/Data/DT_Objectives"));
		return false;
	}

	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FZP_ObjectiveDef* Row = reinterpret_cast<const FZP_ObjectiveDef*>(Pair.Value);
		if (!Row) continue;
		FZP_ObjectiveDef Def = *Row;
		if (Def.Id.IsNone()) Def.Id = Pair.Key; // fall back to row name only if the Id column was omitted
		if (Definitions.ContainsByPredicate([&Def](const FZP_ObjectiveDef& D){ return D.Id == Def.Id; }))
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveSubsystem: DUPLICATE objective id '%s' — skipping"), *Def.Id.ToString());
			continue;
		}
		if (Def.bStartOnLoad) StartOnLoadIds.Add(Def.Id);
		Definitions.Add(Def);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveSubsystem: loaded %d objective rows (%d start-on-load) from DT_Objectives"),
		Definitions.Num(), StartOnLoadIds.Num());
	return true;
}

const FZP_ObjectiveDef* UZP_ObjectiveSubsystem::FindDef(FName Id) const
{
	return Definitions.FindByPredicate([Id](const FZP_ObjectiveDef& D) { return D.Id == Id; });
}

const FZP_SubObjectiveDef* UZP_ObjectiveSubsystem::FindSubDef(FName SubId, const FZP_ObjectiveDef** OutOwner) const
{
	for (const FZP_ObjectiveDef& Def : Definitions)
	{
		for (const FZP_SubObjectiveDef& Sub : Def.SubObjectives)
		{
			if (Sub.Id == SubId)
			{
				if (OutOwner) *OutOwner = &Def;
				return &Sub;
			}
		}
	}
	if (OutOwner) *OutOwner = nullptr;
	return nullptr;
}

bool UZP_ObjectiveSubsystem::AreRequirementsMet(const TArray<FZP_ObjectiveRequirement>& Reqs) const
{
	if (Reqs.Num() == 0) return false; // a stage/reveal with no requirements advances only via an explicit front-end
	for (const FZP_ObjectiveRequirement& Req : Reqs)
	{
		if (!EvaluateRequirement(Req)) return false;
	}
	return true;
}

bool UZP_ObjectiveSubsystem::EvaluateRequirement(const FZP_ObjectiveRequirement& Req) const
{
	switch (Req.Type)
	{
	case EZP_ObjReqType::FlagSet:              return Flags.Contains(FName(*Req.Value));
	case EZP_ObjReqType::ObjectiveComplete:    return CompletedObjectives.Contains(FName(*Req.Value));
	case EZP_ObjReqType::SubObjectiveComplete: return CompletedSubObjectives.Contains(FName(*Req.Value));
	case EZP_ObjReqType::HasItem:              return PlayerHasItem(Req.Value); // Value = PDA_Item asset path
	default:
		// HeardDialogue / CollectedNote / ReachedTrigger are evaluated by their owning systems (M3);
		// until then those sub-objectives are completed explicitly via a front-end.
		return false;
	}
}

void UZP_ObjectiveSubsystem::StartObjective(FName ObjectiveId)
{
	if (ObjectiveId.IsNone() || ActiveObjectives.Contains(ObjectiveId) || CompletedObjectives.Contains(ObjectiveId)) return;

	if (const FZP_ObjectiveDef* Def = FindDef(ObjectiveId))
	{
		for (const FName& Prereq : Def->Requires)
		{
			if (!CompletedObjectives.Contains(Prereq))
			{
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] Objective '%s' blocked — prerequisite '%s' not complete"),
					*ObjectiveId.ToString(), *Prereq.ToString());
				return;
			}
		}
	}

	ActiveObjectives.Add(ObjectiveId);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Objective STARTED: %s"), *ObjectiveId.ToString());
	OnObjectiveStarted.Broadcast(ObjectiveId);
	TryAdvance();
}

void UZP_ObjectiveSubsystem::CompleteObjective(FName ObjectiveId)
{
	if (ObjectiveId.IsNone() || CompletedObjectives.Contains(ObjectiveId)) return;

	ActiveObjectives.Remove(ObjectiveId);
	CompletedObjectives.Add(ObjectiveId);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Objective COMPLETED: %s"), *ObjectiveId.ToString());
	OnObjectiveCompleted.Broadcast(ObjectiveId);
	TryAdvance();
}

void UZP_ObjectiveSubsystem::CompleteSubObjective(FName SubObjectiveId)
{
	if (SubObjectiveId.IsNone() || CompletedSubObjectives.Contains(SubObjectiveId)) return;

	// Explicit completion jumps a (possibly multi-stage, possibly hidden) sub straight to done:
	// reveal it and mark every stage complete so queries/HUD stay consistent.
	RevealedSubObjectives.Add(SubObjectiveId);
	if (const FZP_SubObjectiveDef* Sub = FindSubDef(SubObjectiveId))
	{
		SubObjectiveStage.FindOrAdd(SubObjectiveId) = Sub->GetEffectiveStages().Num();
	}
	CompletedSubObjectives.Add(SubObjectiveId);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Sub-objective COMPLETED: %s"), *SubObjectiveId.ToString());
	OnSubObjectiveCompleted.Broadcast(SubObjectiveId);
	TryAdvance();
}

void UZP_ObjectiveSubsystem::SetFlag(FName Flag)
{
	if (Flag.IsNone() || Flags.Contains(Flag)) return;

	Flags.Add(Flag);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Flag SET: %s"), *Flag.ToString());
	OnFlagSet.Broadcast(Flag);
	TryAdvance();
}

void UZP_ObjectiveSubsystem::TryAdvance()
{
	if (bAdvancing) return;
	TGuardValue<bool> Guard(bAdvancing, true);

	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;

		// 1) Reveal hidden subs, advance multi-stage subs, complete subs whose final stage is met.
		for (const FName& ActiveId : TSet<FName>(ActiveObjectives))
		{
			const FZP_ObjectiveDef* Def = FindDef(ActiveId);
			if (!Def) continue;

			for (const FZP_SubObjectiveDef& Sub : Def->SubObjectives)
			{
				// 1a) Reveal: an always-visible sub (no RevealRequirements) is revealed the moment its
				//     main is active; a hidden sub is revealed once all its RevealRequirements are met.
				if (!RevealedSubObjectives.Contains(Sub.Id))
				{
					const bool bReveal = (Sub.RevealRequirements.Num() == 0) || AreRequirementsMet(Sub.RevealRequirements);
					if (bReveal)
					{
						RevealedSubObjectives.Add(Sub.Id);
						bChanged = true;
					}
					else
					{
						continue; // still hidden — don't evaluate its stages yet
					}
				}

				if (CompletedSubObjectives.Contains(Sub.Id)) continue;

				// 1b) Stage advance: walk forward from the current (monotonic) stage through every stage
				//     whose requirements are now met. Stage progress only ever increases, so a consumed
				//     key item (HasItem now false) can never roll the step back to an earlier title.
				//     A non-final stage with NO requirements is a title-only step and auto-advances; an
				//     empty FINAL stage stays "front-end only" (preserves single-stage semantics — such a
				//     sub completes only via an explicit CompleteSubObjective). Non-final stages must use
				//     an implemented requirement type (FlagSet/HasItem/Objective/SubObjective complete);
				//     HeardDialogue/CollectedNote/ReachedTrigger evaluate false until M3 and would stall.
				const TArray<FZP_ObjectiveStage> Stages = Sub.GetEffectiveStages();
				int32& Stage = SubObjectiveStage.FindOrAdd(Sub.Id); // value = number of stages completed
				while (Stage < Stages.Num())
				{
					const bool bLastStage = (Stage == Stages.Num() - 1);
					const TArray<FZP_ObjectiveRequirement>& StageReqs = Stages[Stage].Requirements;
					const bool bMet = (StageReqs.Num() == 0 && !bLastStage) || AreRequirementsMet(StageReqs);
					if (!bMet) break;
					++Stage;
					bChanged = true;
				}

				// 1b-ii) Raise the current stage's EnterFlag once (idempotent via Flags.Contains) — fires
				//        when the step becomes/while it is active, so it can unlock other systems (e.g. a
				//        transit floor's RequiredFlag) the instant the step is reached. Set directly (not
				//        via SetFlag) to avoid re-entering TryAdvance; bChanged re-runs the settle loop.
				if (Stage < Stages.Num() && !Stages[Stage].EnterFlag.IsNone() && !Flags.Contains(Stages[Stage].EnterFlag))
				{
					Flags.Add(Stages[Stage].EnterFlag);
					OnFlagSet.Broadcast(Stages[Stage].EnterFlag);
					bChanged = true;
				}

				// 1c) Complete the sub once its last stage is done.
				if (Stage >= Stages.Num())
				{
					CompletedSubObjectives.Add(Sub.Id);
					OnSubObjectiveCompleted.Broadcast(Sub.Id);
					bChanged = true;
				}
			}

			// 2) Complete the main once every sub that is in play is done. "In play" = always-visible OR
			//    already revealed; a still-hidden optional sub never blocks completion.
			if (Def->SubObjectives.Num() > 0 && !CompletedObjectives.Contains(ActiveId))
			{
				bool bAllSubs = true;
				for (const FZP_SubObjectiveDef& Sub : Def->SubObjectives)
				{
					const bool bInPlay = (Sub.RevealRequirements.Num() == 0) || RevealedSubObjectives.Contains(Sub.Id);
					if (bInPlay && !CompletedSubObjectives.Contains(Sub.Id)) { bAllSubs = false; break; }
				}
				if (bAllSubs)
				{
					ActiveObjectives.Remove(ActiveId);
					CompletedObjectives.Add(ActiveId);
					OnObjectiveCompleted.Broadcast(ActiveId);
					bChanged = true;
				}
			}
		}

		// 3) Auto-start gated mains whose prerequisites are now all complete.
		for (const FZP_ObjectiveDef& Def : Definitions)
		{
			if (Def.Requires.Num() == 0) continue; // ungated mains are started explicitly
			if (ActiveObjectives.Contains(Def.Id) || CompletedObjectives.Contains(Def.Id)) continue;
			bool bReady = true;
			for (const FName& Prereq : Def.Requires)
			{
				if (!CompletedObjectives.Contains(Prereq)) { bReady = false; break; }
			}
			if (bReady)
			{
				ActiveObjectives.Add(Def.Id);
				OnObjectiveStarted.Broadcast(Def.Id);
				bChanged = true;
			}
		}
	}

	// One refresh pulse after settling — the HUD tracker rebuilds + re-shows (8s) off this.
	OnTrackerRefresh.Broadcast();

	// Persist the settled state (skipped while restoring from a save to avoid a redundant write).
	if (bAutoPersist && !bRestoring)
	{
		SaveObjectiveState();
	}
}

bool UZP_ObjectiveSubsystem::IsObjectiveComplete(FName ObjectiveId) const { return CompletedObjectives.Contains(ObjectiveId); }
bool UZP_ObjectiveSubsystem::IsObjectiveActive(FName ObjectiveId) const { return ActiveObjectives.Contains(ObjectiveId); }
bool UZP_ObjectiveSubsystem::IsSubObjectiveComplete(FName SubObjectiveId) const { return CompletedSubObjectives.Contains(SubObjectiveId); }
bool UZP_ObjectiveSubsystem::IsSubObjectiveRevealed(FName SubObjectiveId) const { return RevealedSubObjectives.Contains(SubObjectiveId); }
int32 UZP_ObjectiveSubsystem::GetSubObjectiveStage(FName SubObjectiveId) const { return SubObjectiveStage.FindRef(SubObjectiveId); }
bool UZP_ObjectiveSubsystem::HasFlag(FName Flag) const { return Flags.Contains(Flag); }

bool UZP_ObjectiveSubsystem::GetActiveObjective(FZP_ObjectiveDef& OutDef) const
{
	for (const FZP_ObjectiveDef& Def : Definitions)
	{
		if (ActiveObjectives.Contains(Def.Id)) { OutDef = Def; return true; }
	}
	return false;
}

bool UZP_ObjectiveSubsystem::GetActiveMainObjective(FZP_ObjectiveDef& OutDef) const
{
	for (const FZP_ObjectiveDef& Def : Definitions)
	{
		if (!Def.bSideObjective && ActiveObjectives.Contains(Def.Id)) { OutDef = Def; return true; }
	}
	return false;
}

bool UZP_ObjectiveSubsystem::GetActiveMainObjectiveView(FZP_ObjectiveDef& OutDef, TArray<FZP_ObjectiveRow>& OutRows) const
{
	OutRows.Reset();
	if (!GetActiveMainObjective(OutDef)) return false;

	for (const FZP_SubObjectiveDef& Sub : OutDef.SubObjectives)
	{
		// Show only revealed steps; drop completed ones so a finished step fades out and the rest remain.
		if (!RevealedSubObjectives.Contains(Sub.Id)) continue;
		if (CompletedSubObjectives.Contains(Sub.Id)) continue;

		const TArray<FZP_ObjectiveStage> Stages = Sub.GetEffectiveStages();
		const int32 Idx = FMath::Clamp(SubObjectiveStage.FindRef(Sub.Id), 0, FMath::Max(0, Stages.Num() - 1));

		FZP_ObjectiveRow Row;
		Row.SubId = Sub.Id;
		Row.Title = Stages.IsValidIndex(Idx) ? Stages[Idx].Title : Sub.Title;
		Row.bComplete = false; // completed subs are omitted above
		OutRows.Add(Row);
	}
	return true;
}

void UZP_ObjectiveSubsystem::NotifyMenuClosed()
{
	OnTrackerRefresh.Broadcast();
}

bool UZP_ObjectiveSubsystem::GetObjectiveDef(FName ObjectiveId, FZP_ObjectiveDef& OutDef) const
{
	if (const FZP_ObjectiveDef* Def = FindDef(ObjectiveId)) { OutDef = *Def; return true; }
	return false;
}

void UZP_ObjectiveSubsystem::GetSaveState(TArray<FName>& OutActive, TArray<FName>& OutCompleted, TArray<FName>& OutCompletedSubs, TArray<FName>& OutFlags) const
{
	OutActive = ActiveObjectives.Array();
	OutCompleted = CompletedObjectives.Array();
	OutCompletedSubs = CompletedSubObjectives.Array();
	OutFlags = Flags.Array();
}

void UZP_ObjectiveSubsystem::RestoreSaveState(const TArray<FName>& InActive, const TArray<FName>& InCompleted, const TArray<FName>& InCompletedSubs, const TArray<FName>& InFlags)
{
	TGuardValue<bool> Guard(bRestoring, true);
	ActiveObjectives = TSet<FName>(InActive);
	CompletedObjectives = TSet<FName>(InCompleted);
	CompletedSubObjectives = TSet<FName>(InCompletedSubs);
	Flags = TSet<FName>(InFlags);
	// This legacy 4-set path carries no reveal/stage data — clear it and let TryAdvance re-derive
	// flag-based reveals/stages. (The active path is WriteToSave/ReadFromSave, which round-trips both.)
	RevealedSubObjectives.Reset();
	SubObjectiveStage.Reset();
	TryAdvance();
}

void UZP_ObjectiveSubsystem::ReevaluateObjectives()
{
	TryAdvance();
}

void UZP_ObjectiveSubsystem::ResetAllProgress()
{
	ActiveObjectives.Reset();
	CompletedObjectives.Reset();
	CompletedSubObjectives.Reset();
	RevealedSubObjectives.Reset();
	SubObjectiveStage.Reset();
	Flags.Reset();

	if (UGameplayStatics::DoesSaveGameExist(ObjectiveStateSlot, 0))
	{
		UGameplayStatics::DeleteGameInSlot(ObjectiveStateSlot, 0);
	}

	// Re-start the auto-start objectives so the tracker isn't left empty (TryAdvance re-persists).
	for (const FName& Id : StartOnLoadIds)
	{
		StartObjective(Id);
	}
	OnTrackerRefresh.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveSubsystem: ALL progress reset (slot '%s' cleared)"), *ObjectiveStateSlot);
}

void UZP_ObjectiveSubsystem::NotifyInventoryChanged(FName ItemId)
{
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveSubsystem: inventory changed (item '%s') — re-evaluating"), *ItemId.ToString());
	TryAdvance(); // HasItem requirements query live inventory; re-check now that it changed
}

bool UZP_ObjectiveSubsystem::PlayerHasItem(const FString& ItemPath) const
{
	if (ItemPath.IsEmpty()) return false;

	UObject* TargetDA = FSoftObjectPath(ItemPath).TryLoad();
	if (!TargetDA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] Objective HasItem: could not load item asset '%s'"), *ItemPath);
		return false;
	}

	const UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	if (!World) return false;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (!Grace || !Grace->MoonvilleInventoryComp) return false;
	UActorComponent* InvComp = Grace->MoonvilleInventoryComp;

	// Reflection search of a Moonville slot array for the target item DataAsset
	// (mirrors AZP_CardReaderPanel::HasItemInSlotArray — the project's existing item-check idiom).
	auto SearchArray = [TargetDA, InvComp](const FName& ArrayName) -> bool
	{
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(InvComp->GetClass()->FindPropertyByName(ArrayName));
		if (!ArrayProp) return false;
		FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
		if (!StructInner) return false;

		FObjectProperty* ItemProp = nullptr;
		for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Item_")))
			{
				if (FObjectProperty* OP = CastField<FObjectProperty>(*It)) { ItemProp = OP; break; }
			}
		}
		if (!ItemProp) return false;

		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InvComp));
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			UObject* SlotItem = ItemProp->GetObjectPropertyValue(ItemProp->ContainerPtrToValuePtr<void>(Helper.GetRawPtr(i)));
			if (SlotItem == TargetDA) return true;
		}
		return false;
	};

	return SearchArray(FName("ItemSlots")) || SearchArray(FName("ShortcutSlots"));
}

void UZP_ObjectiveSubsystem::WriteToSave(UZP_SaveGame* Save) const
{
	if (!Save) return;
	Save->ActiveObjectives        = ActiveObjectives.Array();
	Save->CompletedObjectives     = CompletedObjectives.Array();
	Save->CompletedSubObjectives  = CompletedSubObjectives.Array();
	Save->RevealedSubObjectives   = RevealedSubObjectives.Array();
	Save->SubObjectiveStages      = SubObjectiveStage;
	Save->ProgressFlags           = Flags.Array();
}

void UZP_ObjectiveSubsystem::ReadFromSave(const UZP_SaveGame* Save)
{
	if (!Save) return;
	TGuardValue<bool> Guard(bRestoring, true);
	ActiveObjectives       = TSet<FName>(Save->ActiveObjectives);
	CompletedObjectives    = TSet<FName>(Save->CompletedObjectives);
	CompletedSubObjectives = TSet<FName>(Save->CompletedSubObjectives);
	RevealedSubObjectives  = TSet<FName>(Save->RevealedSubObjectives);
	SubObjectiveStage      = Save->SubObjectiveStages;
	Flags                  = TSet<FName>(Save->ProgressFlags);
	TryAdvance(); // monotonic: never regresses restored stage/reveal progress, only carries it forward
}

void UZP_ObjectiveSubsystem::SaveObjectiveState()
{
	UZP_SaveGame* Save = Cast<UZP_SaveGame>(UGameplayStatics::CreateSaveGameObject(UZP_SaveGame::StaticClass()));
	if (!Save) return;
	WriteToSave(Save);
	UGameplayStatics::SaveGameToSlot(Save, ObjectiveStateSlot, 0);
}

void UZP_ObjectiveSubsystem::LoadObjectiveState()
{
	if (!UGameplayStatics::DoesSaveGameExist(ObjectiveStateSlot, 0)) return;
	UZP_SaveGame* Save = Cast<UZP_SaveGame>(UGameplayStatics::LoadGameFromSlot(ObjectiveStateSlot, 0));
	if (!Save) return;
	ReadFromSave(Save);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveSubsystem: restored objective state from slot '%s' (%d active, %d done)"),
		*ObjectiveStateSlot, ActiveObjectives.Num(), CompletedObjectives.Num());
}
