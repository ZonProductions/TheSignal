// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveReactor.h"
#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "ZP_ObjectiveSubsystem.h"

AZP_ObjectiveReactor::AZP_ObjectiveReactor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AZP_ObjectiveReactor::BeginPlay()
{
	Super::BeginPlay();

	if (AZP_StartupSound)
	{
		UGameplayStatics::PlaySound2D(this, AZP_StartupSound);
	}

	UGameInstance* GI = GetGameInstance();
	UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
	if (!Obj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveReactor %s: no objective subsystem."), *GetName());
		return;
	}

	// Per-level campaign bootstrap: start this level's main objective (once).
	if (AZP_StartObjectiveOnBeginPlay != NAME_None
		&& !Obj->IsObjectiveActive(AZP_StartObjectiveOnBeginPlay)
		&& !Obj->IsObjectiveComplete(AZP_StartObjectiveOnBeginPlay))
	{
		Obj->StartObjective(AZP_StartObjectiveOnBeginPlay);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveReactor %s: started level objective '%s'"),
			*GetName(), *AZP_StartObjectiveOnBeginPlay.ToString());
	}

	// A save restore fired AFTER BeginPlay (EGUI load hook) wholesale-replaces objective state and
	// can wipe the just-started level main — re-assert it whenever a restore lands.
	if (AZP_StartObjectiveOnBeginPlay != NAME_None)
	{
		Obj->OnStateRestored.AddDynamic(this, &AZP_ObjectiveReactor::OnObjectiveStateRestored);
	}

	if (AZP_ListenId == NAME_None) { return; }

	// Already completed before this level loaded — END state instantly, never replay.
	if (Obj->HasFlag(AZP_ListenId) || Obj->IsObjectiveComplete(AZP_ListenId))
	{
		React(/*bInstant*/ true);
		return;
	}
	Obj->OnFlagSet.AddDynamic(this, &AZP_ObjectiveReactor::OnObjectiveEventFired);
	Obj->OnObjectiveCompleted.AddDynamic(this, &AZP_ObjectiveReactor::OnObjectiveEventFired);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveReactor %s: armed on '%s'"),
		*GetName(), *AZP_ListenId.ToString());
}

void AZP_ObjectiveReactor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
		{
			Obj->OnFlagSet.RemoveDynamic(this, &AZP_ObjectiveReactor::OnObjectiveEventFired);
			Obj->OnObjectiveCompleted.RemoveDynamic(this, &AZP_ObjectiveReactor::OnObjectiveEventFired);
			Obj->OnStateRestored.RemoveDynamic(this, &AZP_ObjectiveReactor::OnObjectiveStateRestored);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AZP_ObjectiveReactor::OnObjectiveEventFired(FName Id)
{
	if (Id == AZP_ListenId && !bFired)
	{
		React(/*bInstant*/ false);
	}
}

void AZP_ObjectiveReactor::OnObjectiveStateRestored()
{
	UGameInstance* GI = GetGameInstance();
	UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
	if (!Obj || AZP_StartObjectiveOnBeginPlay == NAME_None) { return; }

	if (!Obj->IsObjectiveActive(AZP_StartObjectiveOnBeginPlay)
		&& !Obj->IsObjectiveComplete(AZP_StartObjectiveOnBeginPlay))
	{
		Obj->StartObjective(AZP_StartObjectiveOnBeginPlay);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveReactor %s: re-asserted level objective '%s' after save restore"),
			*GetName(), *AZP_StartObjectiveOnBeginPlay.ToString());
	}
}

void AZP_ObjectiveReactor::React(bool bInstant)
{
	bFired = true;

	if (!bInstant && AZP_CompleteSound)
	{
		UGameplayStatics::PlaySound2D(this, AZP_CompleteSound);
	}

	if (!bAZP_RetintLights) { return; }

	FadingLights.Reset();
	int32 Matched = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TArray<ULightComponent*> Lights;
		It->GetComponents(Lights);
		for (ULightComponent* L : Lights)
		{
			const FColor C = L->LightColor;
			if (C.R >= AZP_MatchMinRed
				&& C.G <= C.R * AZP_MatchDominance
				&& C.B <= C.R * AZP_MatchDominance)
			{
				++Matched;
				if (bInstant || AZP_LightFadeTime <= 0.f)
				{
					L->SetLightColor(AZP_LightTargetColor);
				}
				else
				{
					FFadingLight F;
					F.Light = L;
					F.From = FLinearColor::FromSRGBColor(C);
					FadingLights.Add(F);
				}
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveReactor %s: '%s' fired — %d light(s) -> target%s"),
		*GetName(), *AZP_ListenId.ToString(), Matched, bInstant ? TEXT(" (instant end-state)") : TEXT(""));

	if (FadingLights.Num() > 0)
	{
		FadeElapsed = 0.f;
		SetActorTickEnabled(true);
	}
}

void AZP_ObjectiveReactor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FadeElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(FadeElapsed / FMath::Max(AZP_LightFadeTime, 0.01f), 0.f, 1.f);
	for (const FFadingLight& F : FadingLights)
	{
		if (F.Light.IsValid())
		{
			F.Light->SetLightColor(FMath::Lerp(F.From, AZP_LightTargetColor, Alpha));
		}
	}
	if (Alpha >= 1.f)
	{
		FadingLights.Reset();
		SetActorTickEnabled(false);
	}
}
