# GDD_Current.md — in-repo working mirror

Per the CLAUDE.md GDD SYNC RULE: any built mechanic is documented here before the next system
begins. The canonical GDD lives externally as .docx (v0.2.1); this file mirrors ONLY what has
been built in-repo since. (File created 2026-07-13 — earlier systems are documented in the
canonical .docx and the /checkpoints/ KB; add them here as they change.)

---

## Enemies

### Oozeling (Enemy 3 — BigBlob pack) — built 2026-07-13
A wall-crawling kamikaze slime. Patrols an authored climb path (AZP_OozeClimbPath spline with
per-point wall normals — walls and ceilings), or wanders the navmesh if no path is assigned.

- **Detection:** range + two-way line of sight; grounded aggro additionally requires navmesh
  reachability (closed doors block it). Wall/ceiling aggro is LOS-only.
- **Aggro routing:** on a ceiling → drops instantly, rights itself, chases on landing; on a
  wall → sprints down the spline to its ground end, then chases; grounded → chases directly
  (navmesh pathing, with straight-line pursuit as fallback where navmesh is holed).
- **THE ATTACK — touch burst with an eruption fuse:** it does not swing. On contact it
  latches on and starts erupting — a vibrating telegraph lasting AZP_TouchFuseTime (default
  1s, "the touch-to-damage time"). If the fuse completes with the player still inside
  AZP_BurstRadius, it bursts: its death animation (Death2) plays — the attack kills it —
  and the player is coated in dissolving ooze: 5% max HP per second for 5 seconds (~25%).
  All tunable (AZP_TouchRange / TouchFuseTime / BurstRadius / TouchDamage*). One Oozeling =
  one burst. The corpse persists (save/load safe, objective-revivable).
- **Death by gunfire:** 5 body shots (flat damage, no headshot zone — it's a blob). Killed
  while wall/ceiling-clinging it falls to the ground first, THEN the death clip plays.
- **Player counterplay:** (1) kill it before it closes (5 body shots at default HP); (2) MELEE
  DEFUSE — kill it during the eruption fuse: clean death, no burst, no DoT (it cannot be
  stagger-interrupted while erupting — only the kill defuses); (3) DODGE — get outside
  AZP_BurstRadius before the fuse ends and it pops harmlessly.
  OPEN DESIGN QUESTION (2026-07-13): DoT ticks currently route through the standard damage
  pipeline, so holding block eats stamina per tick (guard break by tick ~3) — decide whether
  the ooze DoT should be unblockable (dedicated damage type) or keep the stamina pressure.
- Knobs: `Docs/AZP_CustomKnobs.md` → AZP_OozelingBase (41+ knobs). Authoring: place an
  AZP_OozeClimbPath, shape it, click "Snap Wall Normals To Geometry" once, place BP_Oozeling
  nearby (auto-binds the nearest path if the slot is empty).
