# Dodge System — Debug & Reference

_Last updated: 2026-06-20. Covers the dodge rework done in this session
(velocity, stamina, action locks, forward-disable, pipe attachment, camera offsets)._

## Summary

Dodge replaces Jump on the **space bar**. It launches Grace a short distance,
costs stamina, locks out sprint/weapon-swap/reload for the dash, and cannot be
performed in the forward direction (forward = a dash, not a dodge).

Entry point: `AZP_GraceCharacter::PerformDodge()` (called from the space-bar
input handler, which first checks for a ladder dismount).

## Files touched

| File | What changed |
|------|--------------|
| `Source/TheSignal/ZP_GraceCharacter.h` | Dodge knobs (impulse, stamina %, lock window), dodge cam-offset knobs, `DodgeLockRemaining` state |
| `Source/TheSignal/ZP_GraceCharacter.cpp` | `PerformDodge()` rewrite, Tick timer decrement, sprint/reload/weapon-swap gating, dodge camera-offset case |
| `Source/TheSignal/ZP_GraceGameplayComponent.h` | `TryConsumeStaminaPercent()` declaration |
| `Source/TheSignal/ZP_GraceGameplayComponent.cpp` | `TryConsumeStaminaPercent()` implementation |

## Tunable knobs (Details panel)

All live on **BP_GraceCharacter** unless noted.

| Knob | Category | Default | Meaning |
|------|----------|---------|---------|
| `DodgeImpulse` | Dodge | `1200` cm/s | Launch velocity. With ground braking the dash COVERS ~`Impulse²/(2·braking)` cm — ~1200 ≈ a 3 m lunge. Tune down if the spine/camera yanks at high values. |
| `DodgeStaminaCostPercent` | Dodge | `20` | Stamina spent per dodge, as a % of max. Dodge is blocked entirely if you have less than this. |
| `DodgeLockWindow` | Dodge | `0.5` s | How long after a dodge sprint + weapon swap + reload stay locked out. |
| `DodgeCooldown` | Dodge | `0.8` s | Minimum time between dodges. |
| `DodgeClearanceWindow` | Dodge | `0.5` s | How long the dodge camera offset + forward clearance nudge stay active. |
| `Dodge Camera Offset (Melee)` | Camera | `(15,0,0)` | Camera offset while dodging with the pipe up. +X forward (lens away from body), +Z up. Pushes the view clear of the leaning body. |
| `Dodge Camera Offset (Ranged)` | Camera | `(0,0,0)` | Same, while a firearm is up. |
| `DodgeAnim` | Dodge | `FPP_sns_Dodge` | **Currently unused** — left wired for a future pipe-fitted dodge clip (see Pipe section). |

Runtime state (not editable): `DodgeCooldownRemaining`, `DodgeLockRemaining`,
`DodgeClearanceRemaining` — all decremented in `Tick`.

## Behavior details

### Direction gate (forward disabled)

`CurrentMoveInput` is `(X = right [D=+1], Y = forward [W=+1])`, set in
`Input_Move`. In `PerformDodge`:

- Zero input → backstep `(0,-1)`.
- **Blocked**: any forward component — `if (Dir2D.Y > 0.05f) return;`. This
  catches straight-forward AND both forward-diagonals (the dash).
- **Allowed**: pure side (Y≈0), straight back (Y<0), back-diagonals (Y<0).
- The gate runs **before** stamina is spent, so a blocked forward press costs
  nothing.

To side-dodge: hold A/D (not W) when you tap dodge.

### Stamina

`UZP_GraceGameplayComponent::TryConsumeStaminaPercent(Percent)`:
- Converts % to absolute against `MovementConfig->MaxStamina`.
- Returns `false` and spends nothing if `CurrentStamina < cost`.
- On success: subtracts, resets the regen-delay timer (so the dodge actually
  costs you), broadcasts `OnStaminaChanged`.

### Action locks

`DodgeLockRemaining` is set to `DodgeLockWindow` on a successful dodge and
gates these inputs while > 0:
- `Input_SprintStarted` — no sprint mid-dodge.
- `Input_InventorySlot` — no weapon swap / slot use mid-dodge.
- `Input_ReloadStarted` — no reload mid-dodge.

### Launch

`LaunchCharacter(LaunchDir * DodgeImpulse, bXYOverride=true, bZOverride=false)`.
`bXYOverride=true` overrides the CMC input-controller per-tick velocity write —
this is what makes it actually dash (AddImpulse gets eaten by input damping).

### Pipe attachment (melee dodge)

**Known limitation, verified:** there is **no pipe-fitted dodge clip**. The
`A_MeleePipe_*` set (Idle/Block/Attack/Equip/Unequip) has no dodge animation,
and the old `FPP_sns_Dodge` is a sword-and-shield clip whose finger pose doesn't
match the pipe's fitted `hand_r` grip — it floated the pipe off the hand.

Current behavior: on a melee dodge the MeleeViewMesh **holds the seated idle
grip** (`A_MeleePipe_Idle`) so `hand_r` stays at the fitted pose and the pipe
never leaves the hand. The dash reads from body movement + the dodge camera
offset. The pipe is parented to `hand_r` on the MeleeViewMesh
(`ZP_KinemationComponent.cpp` ~line 1134) for its whole life.

If you want actual arm motion on a melee dodge, author/retarget a pipe-fitted
dodge clip and play it via `DodgeAnim` (the slot is still wired).

### Camera offset

In `Tick`, the per-action camera offset state machine picks a target offset
(priority: swing > block > **dodge** > reload > switch), lerped at
`WeaponActionOffsetSpeed` and fed to `GameplayComp->WeaponActionCamOffset`.
The dodge case is active while `DodgeClearanceRemaining > 0` and chooses melee
vs ranged offset by `KinemationComp->CurrentWeaponType`.

There is ALSO a separate forward-clearance nudge (`BlockDodgeForwardClearance`,
14 cm) driven via `SetForwardClearanceActive` during the same window — shared
with block. The dodge cam-offset knobs stack on top of that.

## Debugging

- **Camera probe**: `PerformDodge` sets `CameraProbeTag = "DODGE"` and logs
  `PRE-LAUNCH` / `POST-LAUNCH` camera state, then probes for ~180 frames. Grep
  the log for `DODGE` to trace where the view shifts.
- **No dodge fires?** Check in order: cooldown (`DodgeCooldownRemaining`),
  airborne (`IsFalling`), blocking (`bIsBlocking`), forward direction
  (`Dir2D.Y > 0.05`), insufficient stamina (`TryConsumeStaminaPercent` false).
- **Pipe looks detached on dodge** → it shouldn't anymore; if it does, confirm
  the MeleeViewMesh is on `A_MeleePipe_Idle` during the dash, not a stale clip.
- **Dodge feels like a walk** → `DodgeImpulse` too low relative to sprint speed.
- **Camera clips body on melee dodge** → raise `Dodge Camera Offset (Melee)` +X.

## Related systems

- Block/unblock: same MeleeViewMesh + `hand_r` grip pipeline
  (`UpdateMeleeGrip`, `UpdateBlockAnimation`).
- Stamina/sprint: `UZP_GraceGameplayComponent` (`StartSprint`, `UpdateStamina`).
