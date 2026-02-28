# Checkpoint: Reload Input Working

**Date:** 2026-02-28
**Status:** STABLE — fire + reload both working, arms animated, TR15 in hands

---

## What Was Done

### Reload input wired (same pattern as K001 fire)
- Created `IA_Reload` (Boolean) at `/Game/Core/Input/Actions/IA_Reload`
- Added R key mapping to IMC_Grace (12 total mappings)
- Added `Input_ReloadStarted` handler → `FKinemationBridge::WeaponOnReload(ActiveWeapon)`
- Bound in `SetupPlayerInputComponent` (ETriggerEvent::Started)
- Set `ReloadAction` CDO on BP_GraceCharacter

### Files Modified
| File | Action |
|------|--------|
| `ZP_GraceCharacter.h` | Added `Input_ReloadStarted` declaration |
| `ZP_GraceCharacter.cpp` | Added handler, binding, log format update |
| `Scripts/Python/setup_reload_input.py` | Created — IA_Reload + R key + CDO wiring |

---

## Current Working State

| Feature | Status |
|---------|--------|
| Arms + TR15 visible | Working |
| Movement sway | Working |
| Fire (LMB) | Working |
| Reload (R) | Working |
| Draw animation | Working |
| Aim (RMB) | Working (peek system) |
| Sprint (Shift) | Working |
| Crouch (Ctrl) | Working |
| Jump (Space) | Working |

## Known Issues
- **K002 — Body bleed:** Full body clips into camera when looking down
- **K003 — No lower body:** No visible legs/feet in 1P view, no locomotion animation
- **K004 — No shadow:** No player shadow cast

All three require GASP dual-mesh architecture (TICKET-006 Phase 3).

## IMC_Grace Mappings (12)
WASD, Mouse2D, Shift, Space, E, RMB, LCtrl, LMB, R + originals
