// Copyright The Signal. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZP_GlyphDeviceSubsystem.generated.h"

class FZPGlyphInputProcessor;

/**
 * Purpose: Owns "is the player on gamepad?" and PUSHES it into every live EasyGameUI input-prompt
 *   displayer (WBP_EasyInputPromptDisplayer) via reflection.
 *   DETECTION IS A SLATE INPUT PREPROCESSOR (2026-08-05, 4th iteration): PlayerController-level
 *   key polling is BLIND to gamepad presses while menus are up (Slate consumes them — June-21
 *   documented, log-proven again at 03:16:26), while CommonUI's analog cursor SYNTHESIZES mouse
 *   clicks from those same presses. The preprocessor sees every hardware event BEFORE consumption:
 *   gamepad key/stick -> gamepad; real keyboard key -> KBM; mouse buttons + Enter/Space (the
 *   synth-prone set) -> KBM only if no gamepad event within 0.35s; mouse movement never counts.
 * Owned by: Core / UI input.
 * Extension: none. Knob: zp.GlyphDevice.Enabled (1).
 * Dependencies: Slate (IInputProcessor — see Build.cs note), pack widget members resolved by NAME
 *   at runtime (IsUsingGamepad?, UpdateStyling, UpdateInputIcon) — zero pack edits.
 */
UCLASS()
class THESIGNAL_API UZP_GlyphDeviceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Latest device verdict (gamepad vs KBM) from the Slate preprocessor. Public so other UI
	 *  (e.g. the map legend's glyph brushes) can swap on the SAME signal the EGUI prompts use. */
	bool IsGamepadActive() const { return bGamepadActive; }

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }
	// The pause menu pauses the world — glyph correction must keep running there.
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	void PushToDisplayers(bool bForce);

	TSharedPtr<FZPGlyphInputProcessor> InputProcessor;
	bool bGamepadActive = false;
};
