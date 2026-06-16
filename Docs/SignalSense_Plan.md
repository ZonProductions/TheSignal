# SignalSense — "The Phone" Proximity Warning System

**Status:** PLANNED (2026-06-14). Dev sourcing audio first; no code written yet.
**Owner system:** new — `UZP_SignalSenseComponent` (player-attached).
**Concept origin:** modern Silent Hill radio. The Signal is mute but present through
electronics; the player's phone is its readout. One interference value → three senses
(haptic + audio + HUD waveform).

---

## 1. Behavior — 3 stages

Escalation metaphor: **Notification → Call → Emergency Broadcast.** The phone is the
Signal "trying to reach you," harder each step.

| Stage | Trigger | Phone behavior | Audio cadence | Haptic | Silenceable? |
|-------|---------|----------------|---------------|--------|--------------|
| **0 None** | nothing near | dormant | silent | none | — |
| **1 Voidmote** | voidmote within presence radius | "notification" | one blip every **4–6s (jittered)** | short buzz | **Yes** |
| **2 Enemy near** | enemy within proximity radius | "incoming call" | ring **5s ON / 3s OFF, repeat** | constant rumble during ON | **Yes** |
| **3 Melee** | enemy within melee radius | "emergency broadcast" | **continuous** amber-alert blare (loop) | max continuous | **NO — overrides silence** |

- Stage = highest tier currently true (Melee > Enemy > Voidmote > None).
- **Silence is a risk/reward verb:** muting (stages 1–2) drops your early warning to move
  quiet. Stage 3 punches through the mute on purpose — "you don't get to silence me now."

---

## 2. Audio assets the dev provides (immediate task)

All are **2D / non-spatialized UI sounds** (the phone is on your person, not in the world).
Suggested: 48 kHz, WAV. Naming per project convention (`SFX_`, `MS_`).

| Asset | Stage | Spec |
|-------|-------|------|
| `SFX_Phone_Notify` | 1 | One-shot blip, ~0.3–0.6s, "text notification" feel. **Provide 2–3 variants** to randomize so it never feels metronomic. |
| `SFX_Phone_Ring` | 2 | ~5s ring (or a ringtone motif that fills/loops cleanly to 5s). Played, then 3s silence, repeat. Should feel like a call. |
| `SFX_Signal_Alarm` | 3 | **Seamlessly looping** alarm/WEA blare, ~2–4s loop body, no click at the seam. Plays continuously until stage drops or death. |
| `SFX_AmberAlert_ColdOpen` | intro | **DEFERRED — campaign/cutscene session, NOT in the 24h list.** The breach-moment WEA shriek every phone blares at once. Tied to the opening scripted beat (likely in-engine Sequencer, player keeps first-person control). Build when we do the cold open. |
| `SFX_Phone_Silence` *(opt)* | — | Soft confirmation tone when the player mutes. |
| `MS_Signal_InterferenceBed` *(opt)* | bed | Low ambient interference drone (MetaSound) the waveform idles on and that scales with stage. |

**Note for the waveform (see §4):** if the HUD waveform reads the *actual playing audio's*
amplitude, then "the waveform reacts to the alerts" happens automatically for every sound
above — you author the sounds, the waveform mirrors them for free. Design with that in mind
(clear amplitude envelopes read well as a waveform).

---

## 3. Haptic / controller vibration — the CORE phone sensation

A phone-in-pocket warning is fundamentally a *buzz* — this is arguably the most important
of the four channels (it's the literal Silent Hill radio feel). Fired on the SAME trigger
as the sound + waveform, so all four stay locked in sync (one signal, every sense).

Implemented as **3 `UForceFeedbackEffect` assets** (tunable rumble curves), played by the
component per stage:
- `FF_Signal_Notify` — Stage 1: one short pulse per notify blip ("bzzt," then quiet).
- `FF_Signal_Ring`   — Stage 2: rumble DURING the 5s ring, silent in the 3s gap (buzzes
  like an incoming call). Started/stopped in lockstep with `SFX_Phone_Ring`.
- `FF_Signal_Alarm`  — Stage 3: continuous hard rumble under the alarm (looping).

Played via the PlayerController (`ClientPlayForceFeedback` / `PlayDynamicForceFeedback`).

Honest constraints:
- **Gamepad only.** Keyboard/mouse players feel nothing (no buzz hardware) — for them the
  warning lives in audio + waveform. Expected, not a bug. Real phone haptics only on a
  mobile build.
- **Cannot be demoed standalone** — force feedback only fires in PIE with a controller
  connected. So it lands WITH the component build, alongside the audio wiring.

---

## 4. HUD waveform readout (WBP_HUD)

- A small, clean reactive **waveform line** — NOT a big oscilloscope.
- **Amplitude only, never direction** (Geiger counter, not minimap). Spikes for how-close /
  how-bad; never points which way.
- Driven by a single float `WaveformAmplitude` (0–1) the component exposes.
- **Phase 1 (stub):** drive amplitude off the stage value + a simple oscillation. Ship the
  system end-to-end before fussing the visual.
- **Phase 2 (target):** tap the real UI submix amplitude (UE Audio Synesthesia / runtime
  amplitude analysis) so the waveform literally mirrors whatever sound is playing.
- **Diegetic-vs-always-on fork (UNRESOLVED — dev to decide):** waveform on the phone
  (raise-to-check = vulnerability) vs always-on HUD overlay (readable, less tension).
  Lean: diegetic, with an optional faint always-on baseline pulse.

---

## 5. C++ component spec — `UZP_SignalSenseComponent`

Attach to the player character (`ZP_GraceCharacter` — confirm at build time). C++-first.

**Enum:** `E_SignalStage { None, Voidmote, Enemy, Melee }` (project `E_` prefix).

**State**
- `E_SignalStage CurrentStage`
- `bool bSilenced`
- `float WaveformAmplitude` (drives HUD)

**Tunables — C++ header UPROPERTY defaults (editable in IDE; no DataAsset yet, per dev rule)**
- `VoidmotePresenceRadius`, `EnemyProximityRadius`, `MeleeRadius`
- `Stage1IntervalMin = 4.0`, `Stage1IntervalMax = 6.0`
- `Stage2OnDuration = 5.0`, `Stage2OffDuration = 3.0`
- Detection filters (see §6)

**Logic**
- `EvaluateTimer` @ **0.25s** (timer, NOT Tick — respects no-poll rule): run sphere
  overlaps → compute highest stage → if changed, `SetStage()`.
- `SetStage(New)`: stop prior stage's audio/haptic pattern, start the new one, set
  amplitude target, broadcast `OnSignalStageChanged`.
- Stage patterns via sub-timers:
  - Stage 1: repeating timer, interval jittered in [min,max], fires `PlayNotify()`.
  - Stage 2: 8s repeating cycle — play `SFX_Phone_Ring` + rumble at t=0, stop both at t=5.
  - Stage 3: start looping `SFX_Signal_Alarm` + continuous rumble; no timer.
- Silence gate: `if (CurrentStage < Melee && bSilenced) { suppress audio+haptic; }`
  (keep waveform faint). Input action toggles `bSilenced`; toggling during Stage 3 = no-op.

**Events (event-driven per project rules)**
- `OnSignalStageChanged(E_SignalStage)` multicast delegate — WBP_HUD binds for stage UI.
- `WaveformAmplitude` read by the waveform widget each frame (UI redraw is inherently
  per-frame; acceptable).

---

## 6. Detection of voidmotes / enemies

Two distinct categories (voidmote = Stage 1; enemy = Stages 2–3). Keep flexible:
- Preferred: **GameplayTags** — `Threat.Voidmote`, `Threat.Enemy` — or a tiny interface
  `BPI_SignalDetectable` returning the category. Avoids hard class refs (interface-first).
- Query via `SphereOverlapActors` filtered by tag/interface at the three radii.
- Pick nearest per category; nearest enemy distance decides Enemy-vs-Melee.

---

## 7. Integration / build steps (after audio is ready, on greenlight)

1. Create `UZP_SignalSenseComponent` (.h/.cpp) with the spec above. Build (full
   kill→build→reopen cycle).
2. Add the component to `ZP_GraceCharacter`.
3. Import the SFX assets; assign to the component's sound properties.
4. Tag voidmote + enemy actors (`Threat.Voidmote` / `Threat.Enemy`) or add the interface.
5. WBP_HUD: add the waveform widget; bind `OnSignalStageChanged`; read `WaveformAmplitude`.
6. Bind a "silence phone" input action to toggle `bSilenced`.
7. Stub waveform (Phase 1) → verify all 3 stages drive the right sound/haptic/amplitude →
   upgrade waveform to real-audio-reactive (Phase 2) later.

---

## 8. Open decisions (carry forward)
- Waveform **diegetic vs always-on** (§4).
- Whether Stage 1 reacts to voidmotes **only**, or some enemies too (tuning lever — keep a
  few pure un-warned ambushes for cheap scares).
- Cold-open `SFX_AmberAlert_ColdOpen` staging (scripted breach moment, separate from the
  gameplay loop).
