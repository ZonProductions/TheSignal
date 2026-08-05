// Copyright The Signal. All Rights Reserved.
#include "ZP_GlyphDeviceSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

static TAutoConsoleVariable<int32> CVarZPGlyphDeviceEnabled(
	TEXT("zp.GlyphDevice.Enabled"), 1,
	TEXT("1 = push gamepad/keyboard state into EasyGameUI glyph displayers (Slate-preprocessor detection; mouse movement and synthesized clicks never count as keyboard)."));

/** Sees every hardware input event BEFORE Slate/CommonUI consume or synthesize — the only layer
 *  where "was that a real gamepad press?" is answerable while a menu is up. */
class FZPGlyphInputProcessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		const FKey Key = InKeyEvent.GetKey();
		const double Now = FPlatformTime::Seconds();
		if (Key.IsGamepadKey())
		{
			LastGamepadTime = Now;
		}
		else if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
		{
			// CommonUI gamepad-accept can synthesize these — guarded like mouse buttons.
			LastSynthProneTime = Now;
		}
		else
		{
			LastKeyboardTime = Now;
		}
		return false; // observe only, never consume
	}

	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override
	{
		if (InAnalogInputEvent.GetKey().IsGamepadKey() && FMath::Abs(InAnalogInputEvent.GetAnalogValue()) > 0.25f)
		{
			LastGamepadTime = FPlatformTime::Seconds();
		}
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		LastSynthProneTime = FPlatformTime::Seconds();
		return false;
	}
	// Mouse MOVE deliberately not handled — motion never counts as device choice.

	double LastGamepadTime = -1000.0;
	double LastKeyboardTime = -1000.0;
	double LastSynthProneTime = -1000.0;
};

bool UZP_GlyphDeviceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UZP_GlyphDeviceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FZPGlyphInputProcessor>();
		// INDEX 0 — ahead of CommonUI's preprocessor (log-proven 2026-08-05: CommonUI CONSUMES
		// gamepad keys in menus before later preprocessors see them — G stayed 1.7s stale at the
		// pad press while the synthesized click arrived fresh; every other press was eaten,
		// producing the keyboard/xbox alternation). First in line sees every raw event; we
		// observe-only (return false) so downstream behavior is unchanged.
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
	}
}

void UZP_GlyphDeviceSubsystem::Deinitialize()
{
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();
	Super::Deinitialize();
}

TStatId UZP_GlyphDeviceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZP_GlyphDeviceSubsystem, STATGROUP_Tickables);
}

void UZP_GlyphDeviceSubsystem::Tick(float DeltaTime)
{
	if (!CVarZPGlyphDeviceEnabled.GetValueOnGameThread() || !InputProcessor.IsValid())
	{
		return;
	}

	const double G = InputProcessor->LastGamepadTime;
	const double K = InputProcessor->LastKeyboardTime;
	// Synth-prone events (mouse buttons, Enter, Space) only count as KBM when they did NOT
	// coincide with gamepad activity — a synthesized click always lands within the same instant
	// as the pad press that caused it.
	const double S = (InputProcessor->LastSynthProneTime - G > 0.35) ? InputProcessor->LastSynthProneTime : -1000.0;
	const double KbmTime = FMath::Max(K, S);

	bool bChanged = false;
	if (G > KbmTime && !bGamepadActive)
	{
		bGamepadActive = true;
		bChanged = true;
	}
	else if (KbmTime > G && bGamepadActive)
	{
		bGamepadActive = false;
		bChanged = true;
	}

	// Correct EVERY tick — the pack's own (poisoned) handler still flips widgets on hardware
	// events; per-tick mismatch correction makes that invisible. Writes only on drift.
	PushToDisplayers(/*bForce=*/false);
	if (bChanged)
	{
		UE_LOG(LogTemp, Log, TEXT("[GlyphDevice] device state -> %s (G=%.1f K=%.1f S=%.1f)"),
			bGamepadActive ? TEXT("GAMEPAD") : TEXT("KBM"),
			G > 0 ? FPlatformTime::Seconds() - G : -1.0,
			K > 0 ? FPlatformTime::Seconds() - K : -1.0,
			InputProcessor->LastSynthProneTime > 0 ? FPlatformTime::Seconds() - InputProcessor->LastSynthProneTime : -1.0);
	}
}

void UZP_GlyphDeviceSubsystem::PushToDisplayers(const bool bForce)
{
	static const FName DisplayerClassName(TEXT("WBP_EasyInputPromptDisplayer_C"));
	UWorld* World = GetWorld();
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* W = *It;
		if (!IsValid(W) || W->GetClass()->GetFName() != DisplayerClassName)
		{
			continue;
		}
		if (W->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			continue;
		}
		if (W->GetWorld() != World)
		{
			continue; // only live PIE/game instances, not editor-loaded templates
		}
		FBoolProperty* Prop = CastField<FBoolProperty>(W->GetClass()->FindPropertyByName(FName(TEXT("IsUsingGamepad?"))));
		if (!Prop)
		{
			continue;
		}
		const bool bCurrent = Prop->GetPropertyValue(Prop->ContainerPtrToValuePtr<void>(W));
		if (bCurrent == bGamepadActive && !bForce)
		{
			continue;
		}
		Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(W), bGamepadActive);
		if (UFunction* Fn = W->FindFunction(FName(TEXT("UpdateStyling"))))
		{
			W->ProcessEvent(Fn, nullptr);
		}
		if (UFunction* Fn = W->FindFunction(FName(TEXT("UpdateInputIcon"))))
		{
			W->ProcessEvent(Fn, nullptr);
		}
	}
}
