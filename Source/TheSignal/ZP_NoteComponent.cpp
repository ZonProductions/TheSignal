// Copyright The Signal. All Rights Reserved.

#include "ZP_NoteComponent.h"
#include "ZP_NoteSubsystem.h"
#include "Engine/GameInstance.h"

const TArray<FZP_NoteEntry> UZP_NoteComponent::EmptyNotes;

UZP_NoteComponent::UZP_NoteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_NoteComponent::BeginPlay()
{
	Super::BeginPlay();

	// Resolve the global note subsystem
	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		NoteSubsystem = GI->GetSubsystem<UZP_NoteSubsystem>();
	}

	UE_LOG(LogTemp, Log, TEXT("[NoteComponent] BeginPlay — Subsystem=%s, Notes=%d"),
		NoteSubsystem ? TEXT("valid") : TEXT("NULL"),
		NoteSubsystem ? NoteSubsystem->GetNoteCount() : 0);
}

void UZP_NoteComponent::AddNote(const FZP_NoteEntry& Note)
{
	if (Note.NoteID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NoteComponent] Attempted to add note with empty NoteID — ignored."));
		return;
	}

	if (!NoteSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NoteComponent] No NoteSubsystem — note '%s' lost!"), *Note.NoteID.ToString());
		return;
	}

	if (NoteSubsystem->AddNote(Note))
	{
		// Only broadcast if actually new
		OnNoteAdded.Broadcast(Note);
	}
}

bool UZP_NoteComponent::HasNote(FName NoteID) const
{
	return NoteSubsystem ? NoteSubsystem->HasNote(NoteID) : false;
}

const FZP_NoteEntry* UZP_NoteComponent::GetNote(FName NoteID) const
{
	if (!NoteSubsystem) return nullptr;

	for (const FZP_NoteEntry& Note : NoteSubsystem->GetNotes())
	{
		if (Note.NoteID == NoteID)
		{
			return &Note;
		}
	}
	return nullptr;
}

const TArray<FZP_NoteEntry>& UZP_NoteComponent::GetNotes() const
{
	return NoteSubsystem ? NoteSubsystem->GetNotes() : EmptyNotes;
}

TArray<FZP_NoteEntry> UZP_NoteComponent::GetCodes() const
{
	TArray<FZP_NoteEntry> Codes;
	if (!NoteSubsystem) return Codes;

	for (const FZP_NoteEntry& Note : NoteSubsystem->GetNotes())
	{
		if (Note.bIsCode)
		{
			Codes.Add(Note);
		}
	}
	return Codes;
}
