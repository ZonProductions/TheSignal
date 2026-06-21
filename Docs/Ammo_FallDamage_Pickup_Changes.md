# Ammo / Fall Damage / Firearm Pickup — Change & Debug Doc

_Created 2026-06-20. Tracks the backlog bugs + feature requests in this batch so
each change is debuggable in isolation. Dodge is NOT covered here (see
`Docs/Dodge_System_Debug.md`)._

---

## Scope (what was requested)

### Backlog bugs
1. **Unarmed shows `12 / 48`.** That's the pistol demo-test default. When unarmed
   (no weapon up), the ammo widget must show **nothing** — same as the pipe.
2. **`DA_Ammo_*` (9mm / 556 / buckshot) don't replenish reserve.** Picking up an
   ammo data asset for a gun you own must raise that gun's reserve count in the
   ammo widget (e.g. 45 → pick up 9mm → goes up). Currently blocked / no-op.

### New features
1. **Fall damage.** 2-story drop = 100 HP (lethal). 1-story drop = 30 HP. Linear
   between 30–100 HP across the distance band. Expose **min distance** (below =
   no damage) and **max distance** (at/above = 100 HP) knobs on
   `BP_GraceCharacter`; code interpolates the rest.
2. **Firearm pickup rules.**
   - **2a.** Picking up a firearm loads **only 1 clip/round** in the magazine —
     not the full reserve it currently grants.
   - **2b.** The value currently granted is treated as the **MAX obtainable
     reserve** (cap). At max you **cannot** interact with ammo until the reserve
     in the widget drops below the cap. Caps: **pistol = 48, shotgun = 24,
     assault rifle = 90.**
   - **2c.** A per-pickup knob in **Details** on each placed `BP_ItemPickup` so the
     designer can override the granted/cap amount per instance.
   - **2d.** Cannot interact with / pick up a firearm already in inventory (own a
     shotgun → the other shotgun is non-interactable).

---

## System map (where each thing lives)

| Concern | Location |
|---------|----------|
| Ammo state (`CurrentAmmo`, `ReserveAmmo`, `MagSize`) | `UZP_KinemationComponent` (`ZP_KinemationComponent.h/.cpp`) |
| Per-weapon ammo config (mag/reserve by weapon name) | `ApplyWeaponConfig()` `ZP_KinemationComponent.cpp` ~L565 |
| Add to reserve (ammo pickups) | `AddReserveAmmo()` `ZP_KinemationComponent.cpp` ~L709 |
| Ammo HUD render + visibility by weapon type | `SetAmmo()` / `SetWeaponType()` `ZP_HUDWidget.cpp` ~L199 |
| Weapon type enum (incl. `None`) | `EZP_WeaponType` `ZP_KinemationComponent.h` |
| Pickup / interaction routing | `ZP_GraceCharacter.cpp` interaction trace ~L1390; Moonville `BP_ItemPickup` |
| Health / damage application | `UZP_HealthComponent` |
| Landing detection | `ACharacter::Landed` (override on `AZP_GraceCharacter`) |

---

## Planned changes (per item — fill STATUS as each lands)

### Bug 1 — unarmed ammo display
- **Change:** add a `case EZP_WeaponType::None:` in `UZP_HUDWidget::SetAmmo` that
  collapses `AmmoText` (mirror the `Melee` case). Ensure `UnequipWeapon()`
  resets `CurrentWeaponType = None` and broadcasts `OnWeaponTypeChanged` so the
  HUD blanks on holster, and that the initial unarmed state reports `None`.
- **Why it currently shows 12/48:** `SetAmmo`'s `switch` has only Melee /
  Throwable / Ranged(+default); `None` falls through to the Ranged default and
  prints `CurrentAmmo / ReserveAmmo` (the 12/48 demo defaults).
- **STATUS:** _DONE (2026-06-20)._ Added `case EZP_WeaponType::None:` to
  `UZP_HUDWidget::SetAmmo` (collapses `AmmoText`). `UZP_KinemationComponent::UnequipWeapon`
  now resets `CurrentWeaponType`/`CurrentWeaponIcon` to `None` and broadcasts
  `OnWeaponTypeChanged` + `OnWeaponIconChanged`. Initial state already reports
  `None` (HUD `BindToCharacter` reads `CurrentWeaponType`, which defaults `None`).

### Bug 2 — ammo pickups don't replenish
- **Change:** route `DA_Ammo_*` consumption to `KinemationComp->AddReserveAmmo`
  for the matching weapon type, clamped to the per-weapon cap (see 2b). Verify
  the pickup actually calls into C++ (Moonville consumable hook) rather than
  silently being dropped.
- **STATUS:** _DONE (2026-06-20)._ Root cause confirmed: ammo items had no action
  and nothing routed them to reserve (they just sat in the grid; lockers even
  stripped them). Fix is C++-only, no Moonville BP edit needed:
  `AZP_GraceCharacter::ConvertAmmoItemsToReserve()` runs off `OnInventoryUpdate`
  (so world/locker/container pickups all count), scans `ItemSlots` for `*Ammo*`
  items, and for each ammo the player owns a gun for, banks the stack into that
  gun's reserve via `KinemationComp->AddAmmoForIcon` (cap-clamped) then removes
  the item. Re-entrancy guarded (removal re-fires the dispatcher). Ammo→weapon
  map: `9mm`→Pistol, `Buckshot`→Shotgun, `556`→Rifle. **At cap the ammo is left
  in the bag (not eaten)** — converts later when there's room.

### Feature 1 — fall damage
- **Change:** override `AZP_GraceCharacter::Landed(const FHitResult&)`. Track
  fall start Z (on leaving ground / start of fall), compute drop distance on
  land, map linearly: `dist <= FallDamageMinDistance` → 0; `dist >=
  FallDamageMaxDistance` → 100 (lethal); between → lerp 30→100. Apply via
  `HealthComp` / `TakeDamage`.
- **Knobs (Details → "Fall Damage" on BP_GraceCharacter):**
  `FallDamageMinDistance`, `FallDamageMaxDistance`. (1-story / 2-story tuned by
  the designer to map to the 30 / 100 endpoints.)
- **STATUS:** _DONE (2026-06-20)._ Overrode `Landed` + `OnMovementModeChanged` on
  `AZP_GraceCharacter`. `OnMovementModeChanged` seeds fall tracking on entering
  `MOVE_Falling`; `Tick` records the apex Z; `Landed` computes
  `drop = peakZ - landingZ`, applies `<=min → 0`, `>=max → 100`, lerps
  `FallDamageMinAmount→MaxAmount` between, via `HealthComp->ApplyDamage` (bypasses
  block reduction — a fall isn't blockable). Knobs: `FallDamageMinDistance` (300),
  `FallDamageMaxDistance` (600), `FallDamageMinAmount` (30), `FallDamageMaxAmount`
  (100), all `EditAnywhere` on `BP_GraceCharacter` → Details → "Fall Damage".
  **Defaults are placeholders — tune Min/Max distance to the actual floor heights.**

### Feature 2a/2b — pickup grants 1 clip, reserve is capped
- **Change:** on firearm pickup, set `CurrentAmmo = MagSize` (one clip) and do
  NOT dump the full reserve. Treat the existing per-weapon reserve numbers as
  the **cap**. `AddReserveAmmo` clamps `ReserveAmmo` to the cap. Pistol 48 /
  shotgun 24 / rifle 90 (assault). Ammo interaction blocked while at cap.
- **STATUS:** _DONE (2026-06-20)._ Solved the swap-persistence problem with
  **per-weapon reserve pools** (`ReservePools: TMap<EZP_WeaponIcon,int32>` on
  `UZP_KinemationComponent`). The EQUIPPED weapon's reserve lives in `ReserveAmmo`;
  `ApplyWeaponConfig`/`UnequipWeapon` bank it to/load it from the pool per icon, so
  each gun keeps its own reserve across switches (pick up rifle ammo while holding
  the pistol → the rifle's reserve rises, not the pistol's).
  **2a:** fresh pickup of a gun → pool empty → `ReserveAmmo = 0` and `CurrentAmmo =
  MagSize` (1 clip in the mag, empty reserve until ammo is found).
  **2b:** cap is icon-driven via `GetReserveCapForIcon` (Pistol 48 / Shotgun 24 /
  Rifle 90); `AddAmmoForIcon` clamps to it. TR15→Rifle (90), SRM→Shotgun (24).
  **Note:** mag still refills to full on every equip (pre-existing; out of scope).

### Feature 2c — per-pickup override knob
- **Change:** expose an `EditAnywhere` amount on `BP_ItemPickup` so each placed
  instance can override the granted/cap value.
- **STATUS:** _pending (Moonville BP edit)_

### Feature 2d — no duplicate firearm pickup
- **Change:** in the interaction gate, if the player already owns a weapon of the
  same class/type, make the duplicate pickup non-interactable (no prompt, no
  pickup).
- **STATUS:** _DONE (2026-06-20)._ `ShouldBlockPickupInteraction(AActor*)` reads the
  pickup's `Item` DA (`GetPickupItemDA`): if it's a weapon already owned
  (`CountWeaponClassInInventory > 0`) the pickup is gated — pressing E does nothing.
  Applied in BOTH interaction paths (crosshair trace + Moonville proximity fallback).
  **Throwables (Grenade/Rock) are excepted** — they stack, so more can still be
  grabbed. Same gate also enforces **2b at the interaction level**: ammo at cap for
  its type can't be picked up. **Caveat:** Moonville still draws its "Press E" popup
  from its own overlap; the ACTION is blocked but suppressing the popup itself is a
  Moonville-widget change (not done — avoids driving Moonville menu state, session 63).

### Interaction reach (added request 2026-06-20)
- **Change:** `InteractionReach` knob (BP_GraceCharacter → Details → Interaction,
  default 300cm) replaces the hardcoded crosshair-trace distance. ALSO: the trace
  now accepts a `BP_ItemPickup`'s interaction SPHERE (it previously skipped all shape
  components, so only the tiny item mesh was hittable → near-melee reach needed for
  items on tables). Containers/lockers keep the shape skip (their spheres bulge
  through walls — session 63). Now you can grab table items at a normal distance.
- **STATUS:** _DONE (2026-06-20)._

---

## Debugging notes
- Ammo flow is event-driven: every mutation broadcasts
  `OnAmmoChanged(CurrentAmmo, ReserveAmmo)` → `UZP_HUDWidget::OnAmmoChangedHandler`
  → `SetAmmo`. If the widget doesn't update, the broadcast didn't fire or the
  HUD isn't bound (`BindToCharacter`).
- Weapon type / ammo race: equips broadcast **ammo before type**, so `SetAmmo`
  can format with the *previous* weapon's type. `SetWeaponType` re-renders from
  `LastCurrentAmmo/LastReserveAmmo` to fix this — keep that ordering in mind when
  debugging "ammo shows the wrong format right after a swap."
- Per-weapon reserve/mag numbers are matched by **weapon class name substring**
  in `ApplyWeaponConfig` (Viper/Herrington/AK105/TR15/SRM/Pipe/Grenade). New
  guns need an entry there or they fall to the ranged defaults.
