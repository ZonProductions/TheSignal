// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveSubsystem.h"
#include "Engine/DataTable.h"

void UZP_ObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadDefinitions();
	for (const FName& Id : StartOnLoadIds)
	{
		StartObjective(Id);
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

bool UZP_ObjectiveSubsystem::EvaluateRequirement(const FZP_ObjectiveRequirement& Req) const
{
	switch (Req.Type)
	{
	case EZP_ObjReqType::FlagSet:              return Flags.Contains(FName(*Req.Value));
	case EZP_ObjReqType::ObjectiveComplete:    return CompletedObjectives.Contains(FName(*Req.Value));
	case EZP_ObjReqType::SubObjectiveComplete: return CompletedSubObjectives.Contains(FName(*Req.Value));
	default:
		// HasItem / HeardDialogue / CollectedNote / ReachedTrigger are evaluated by their owning
		// systems (M3); until then those sub-objectives are completed explicitly via a front-end.
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

		// 1) Auto-complete sub-objectives whose (self-evaluable) requirements are all met.
		for (const FName& ActiveId : TSet<FName>(ActiveObjectives))
		{
			const FZP_ObjectiveDef* Def = FindDef(ActiveId);
			if (!Def) continue;

			for (const FZP_SubObjectiveDef& Sub : Def->SubObjectives)
			{
				if (CompletedSubObjectives.Contains(Sub.Id) || Sub.Requirements.Num() == 0) continue;
				bool bAll = true;
				for (const FZP_ObjectiveRequirement& Req : Sub.Requirements)
				{
					if (!EvaluateRequirement(Req)) { bAll = false; break; }
				}
				if (bAll)
				{
					CompletedSubObjectives.Add(Sub.Id);
					OnSubObjectiveCompleted.Broadcast(Sub.Id);
					bChanged = true;
				}
			}

			// 2) Complete the main if all its subs are done.
			if (Def->SubObjectives.Num() > 0 && !CompletedObjectives.Contains(ActiveId))
			{
				bool bAllSubs = true;
				for (const FZP_SubObjectiveDef& Sub : Def->SubObjectives)
				{
					if (!CompletedSubObjectives.Contains(Sub.Id)) { bAllSubs = false; break; }
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
}

bool UZP_ObjectiveSubsystem::IsObjectiveComplete(FName ObjectiveId) const { return CompletedObjectives.Contains(ObjectiveId); }
bool UZP_ObjectiveSubsystem::IsObjectiveActive(FName ObjectiveId) const { return ActiveObjectives.Contains(ObjectiveId); }
bool UZP_ObjectiveSubsystem::IsSubObjectiveComplete(FName SubObjectiveId) const { return CompletedSubObjectives.Contains(SubObjectiveId); }
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
	ActiveObjectives = TSet<FName>(InActive);
	CompletedObjectives = TSet<FName>(InCompleted);
	CompletedSubObjectives = TSet<FName>(InCompletedSubs);
	Flags = TSet<FName>(InFlags);
	TryAdvance();
}
