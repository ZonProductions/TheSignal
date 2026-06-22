# Plan — Transit & Campaign Progression System

> ⚠️ **PARTIALLY SUPERSEDED (2026-06-21).** The "campaign progression" half of this doc is OUT OF DATE — it
> describes a GameplayTag-based `UZP_ProgressionSubsystem` that was **not** built. The progression layer became
> the **Objective System** (FName-keyed `UZP_ObjectiveSubsystem`, JSON→DataTable, EasyGameUI HUD tracker) —
> see **`Docs/Plan_ObjectiveSystem.md`** for the as-built design. The TRANSIT half below is still broadly
> accurate as built (`AZP_TransitPanel` shipped instead of `AZP_TransitPoint`; transit gates on objectives/keys
> in M3, which is still pending). Treat tag references here as FName ids.

**Status:** Proposed (planning). Owner subsystem: `SS_Transit` (new) + `SS_Progression` (new).
**Goal:** One reusable "pick a destination" UI for elevators, trams, and building entrances, backed by a
campaign-progression layer that controls which destinations are available. Fallout-4-style simple list now;
scales to every chapter of the game.

---

## 1. The two things the first draft missed (the heart of this doc)

### GAP 1 — A dead-simple way to "add a destination to availability"
There must be **one verb** that any source (note, cutscene, objective, trigger, item) can fire, with **zero
boilerplate**, to make a floor appear in the elevator.

**The verb:** `UZP_ProgressionSubsystem::Unlock(FGameplayTag DestinationTag)` — one Blueprint node, callable
from anywhere. A destination declares an `UnlockTag`; when that tag is granted, the destination flips from
locked → available and any open menu refreshes live (via an `OnProgressGranted` dispatcher).

**Three zero-friction front-ends (designer never writes code):**
1. **Item / note pickup grants it.** The interactable carries an optional `GrantsProgressTag`. Picking up the
   keycard or reading the note auto-unlocks the floor. *(Literally "an item that adds a destination.")*
2. **`AZP_ProgressTrigger` drop-in actor.** Place it, set `TagToGrant`, choose Overlap or Interact. Done.
3. **Single BP node `Unlock(Tag)`** for Sequencer/cutscene callbacks and objective-complete events.

That is the simple authoring loop: **declare `UnlockTag` on the floor → fire `Unlock(Tag)` from one of the
three front-ends.** No per-floor code, no rewiring the elevator.

### GAP 2 — Key-level lock capability
A destination can be gated by a **physical key item** (keycard), not just story progress — reusing the
existing InventorySystem and mirroring the proven door lock/key pattern.

- Per-destination `RequiredKeyItem` (inventory item/tag) + `bConsumeKeyOnUse`.
- At selection: has key → travel (consume if flagged); missing → show `LockedReason` ("Requires Maintenance
  Keycard").
- **Composable** with `UnlockTag`: a floor can need *both* story progress *and* a keycard.
- **Whole-panel lock:** `bEnabled` / `RequiresPowerTag` on the transit point itself ("elevator offline until
  power restored").

Availability is therefore a clean AND of three independent gates: progression tag, key item, panel power.

---

## 2. Components

| Name | Type | Role |
|------|------|------|
| `AZP_TransitPoint` | C++ Actor | Placeable object. Holds the **mesh** (set per placement) + destination list. Implements `BPI_Interactable`; on interact spawns the menu. |
| `WBP_TransitMenu` | Widget BP | Fallout-4-style vertical destination list. Available = clickable; locked = greyed + reason (or hidden). |
| `UZP_ProgressionSubsystem` | GameInstanceSubsystem | Set of unlocked `FGameplayTag`s. `Unlock()`, `HasProgress()`, `OnProgressGranted` dispatcher. Persisted by SaveSystem. The campaign-gating backbone (reused beyond transit). |
| `S_TransitDestination` | Struct | One destination's data (below). |
| `AZP_ProgressTrigger` | C++/BP Actor | Drop-in unlock front-end (overlap/interact → `Unlock(Tag)`). |
| `DA_TransitNetwork` | DataAsset (later) | Shared destination list once multiple points reuse the same floors. |

Integrations: **InventorySystem** (key items), **SaveSystem** (persist progression), **GI_TheSignal**
(carry inventory/state across level load).

---

## 3. Data model — `S_TransitDestination`

- `DisplayName` (Text) — "Floor 3 — Labs"
- `DestinationTag` (GameplayTag) — identity, e.g. `Transit.Building1.Floor3`
- `TargetLevel` (TSoftObjectPtr<World>) — the floor/building map
- `ArrivalPointTag` (Name) — which PlayerStart to spawn at on arrival
- `UnlockTag` (GameplayTag, optional) — progression gate (empty = always open)
- `RequiredKeyItem` (item id/tag, optional) — inventory gate
- `bConsumeKeyOnUse` (bool)
- `LockedReason` (Text) — shown when gated
- `bHideUntilUnlocked` (bool) — hidden vs greyed when locked

**Availability(dest) =** `(UnlockTag empty OR Progression.Has(UnlockTag))` **AND**
`(RequiredKeyItem empty OR Inventory.Has(RequiredKeyItem))` **AND** `TransitPoint.bEnabled`.

---

## 4. Travel
Select → fade → `OpenLevel(TargetLevel)` → spawn at `ArrivalPointTag`. Inventory/state carried by
`GI_TheSignal` + SaveSystem. Faux ride for now (fade out → load → fade in), like FO4. Full level load per
floor/building (matches "elevator parts out levels"); revisit streaming only if loads feel heavy.

---

## 5. Persistence
`UZP_ProgressionSubsystem`'s tag set is written/read by the existing SaveSystem so unlocks survive save/load.
Consumed keys already persist via inventory save.

---

## 6. Architecture compliance
C++ for the bones (`AZP_TransitPoint`, `UZP_ProgressionSubsystem`, struct), BP/widget for UI.
Interface-first (`BPI_Interactable` + dispatchers, no BP-to-BP coupling). Event-driven (no Tick).
GameplayTags instead of magic strings. Naming: `AZP_/UZP_/WBP_/S_/DA_/BPI_`. Destinations live as a
per-actor array now → promote to `DA_TransitNetwork` when shared. **3-system flag acknowledged**
(Interaction + Save + Narrative/Progression + level loading) — this doc is that review.

---

## 7. Build milestones
- **M1 — Ride works.** `AZP_TransitPoint` (mesh + `BPI_Interactable`) + `WBP_TransitMenu` + inline
  destination array + `OpenLevel`. Hard-wire 2–3 real floors, prove end-to-end travel.
- **M2 — Availability.** `UZP_ProgressionSubsystem` + `UnlockTag` gating + `Unlock()` verb + the three
  front-ends (`AZP_ProgressTrigger`, item `GrantsProgressTag`, BP node).
- **M3 — Key lock.** `RequiredKeyItem` + inventory check + consume + locked reasons + panel power lock.
- **M4 — Persistence.** SaveSystem stores/loads progression tags.
- **M5 — Scale.** Promote to `DA_TransitNetwork` when a second point shares floors (trams).

---

## 8. Open decisions (need dev call before/along the way)
1. **Locked floors: greyed-with-reason vs hidden** until unlocked (per-destination default for `bHideUntilUnlocked`).
2. **Destinations: per-actor array now** (recommended M1) vs jump straight to `DA_TransitNetwork`.
3. **Keys: consume-on-use vs permanent** (default per destination).
4. **Travel: `OpenLevel` (recommended)** vs level streaming / World Partition data layers.
5. **Tag taxonomy:** confirm `Transit.<Building>.<Floor>` and `Progress.<...>` naming scheme.
