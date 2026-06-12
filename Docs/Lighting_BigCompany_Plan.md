# Maps_BigCompany — Lighting Diagnosis & Horror Relight Plan
*(Prepared 2026-06-12, session 63. Diagnosis only — no changes made.)*

## DIAGNOSIS — where the "harsh darkness" comes from

Two compounding causes, both verified live:

1. **The level has ZERO light actors.** Session 39 deleted all ~50 interior
   Spot/RectLights AND every fluorescent/lamp fixture mesh while chasing the
   "swimming pool" artifact — which turned out to be Lumen GI, not the
   fixtures (documented wrong-root-cause in memory/lighting.md). The lights
   were never restored. All that remains: 153 LightmassPortals + 2 importance
   volumes (useless without lights), ~60 stale reflection captures.
2. **PPV_GlobalDarkness** (unbound, priority 10) clamps
   `indirect_lighting_intensity = 0.0025` — this floor-crushes all ambient
   fill. Tuned for TreatmentStation's horror bed; with no interior lights on
   top of it, BigCompany reads as pure black.

Also present: `NightSkyAtmosphere` + `NightFog` (night campus ✓), base
PostProcessVolume (priority 0: exposure 0.03–8, bloom 0.6, temp 5800,
vignette 0.25).

**DO NOT delete PPV_GlobalDarkness** — it carries the Lumen-GI-off fix
(deleting it resurrects the swimming-pool artifact). The darkness resolves by
ADDING a light layer, then re-tuning the ambient floor.

## RECOMMENDATION — "after-hours office under failing fluorescents"

Concept fit: corporate tower at night, post-breach. Pools of sickly cool
fluorescent light with real darkness between them. Ren controls electronics —
flickering and dying tubes are diegetic AND narrative (Ren's presence).
Warm flashlight (existing, 7000 warm) vs cold room light = the player's
comfort color against the facility's.

### Light recipe
- **Fixture light:** RectLight per fixture, cool white **6500K with a faint
  green-cyan bias** (institutional dread — SH/REmake grammar). Low intensity,
  tight attenuation (600–900 UU) so pools stay pools.
- **Shadows:** ON only for key/landmark fixtures (movable shadowed rects are
  expensive); filler fixtures unshadowed at lower intensity.
- **Emissive fixture mesh** matching each light (pack has SM_CircleLight,
  SM_HangedLamp, SM_LampBig, SM_SpotLight_Hang, SM_StandLight + tintable
  MI_LightWhite/Blue/Green materials — assets survived the purge; only level
  actors were deleted).
- **Window spill:** faint cool blue at select exterior windows (dim SkyLight
  ~0.05 cool-tinted, or thin RectLights at key windows) — pairs with
  TICKET-052 night-campus backdrop.

### Placement grammar (authored tension, not uniform coverage)
- **Corridors:** fixture every 2nd–3rd ceiling slot — light/dark/light rhythm.
- **Rooms:** ONE fixture near the focal point; corners stay dark.
- **Elevator lobbies + stairwells:** always lit — navigation landmarks.
- **Scare/loot rooms (tech support etc.):** the dying-tube variant or unlit
  (flashlight territory).
- Layers panel already splits floors (F1–F5) — light one floor at a time.

### Flicker system (C++-first rule)
`UZP_FlickerLightComponent` — per-instance seed, three modes:
- `Stable` (no flicker),
- `Nervous` (2–5% intensity wobble, occasional 1-frame dips),
- `Dying` (hard stutters, sputter-off-on cycles, audio hook for buzz SFX).
Ren hook later: a function to drive flicker from narrative events
(RenCommunicationSystem is planned — this is its lighting entry point).

### Grade pass (on PPV_GlobalDarkness, after lights exist)
- Raise `indirect_lighting_intensity` 0.0025 → ~0.004–0.006 (tune by eye,
  `_bump_light.py` workflow).
- `white_temp` ~5500 (pushes the image cold against 6500K lights).
- `color_saturation` ~0.85 (drained institutional look).
- `film_grain_intensity` 0.03–0.05 (matches the menu design language).

### Implementation order (when approved)
1. Build `BP_LightFluorescent` (fixture mesh + RectLight + flicker comp);
   C++ flicker component first.
2. Python placement pass per floor using the ceiling grid (fast, reviewable —
   one floor, dev approves look, then batch the rest).
3. Window spill pass.
4. PPV grade re-tune.
5. Recapture the ~60 reflection captures (one command) — they're stale.
6. Per-floor walkthrough with the dev for the authored dark zones.

### Open decisions for the dev
- Fixture mesh: pack's SM_CircleLight (recessed office) vs SM_HangedLamp
  (industrial pendant) vs importing a true tube fixture.
- Green tint amount: subtle (RE7) vs overt (SH2 sickly).
- Density: how dark is too dark between pools (corridor rhythm spacing).
- Whether Building 1's lower floors differ in mood from upper (escalation).
