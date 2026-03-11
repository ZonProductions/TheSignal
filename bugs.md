# The Signal — Bug Tracker (Cross-Session)

## OPEN BUGS — Must fix before new work

### BUG-001: Equipped weapon not unequipped when placed in briefcase
- **Status:** OPEN
- **Severity:** High
- **Repro:** Equip pistol → open briefcase → place pistol in briefcase → pistol remains equipped (visible, usable) despite being in container inventory
- **Expected:** Placing equipped weapon into container should unequip it (remove from hands, hide mesh, disable firing)
- **Systems:** InventorySystem, WeaponSystem, BP_Briefcase (Moonville transfer UI)

### BUG-002: Multiple adjacent loot lockers — interaction fails
- **Status:** OPEN
- **Severity:** High
- **Repro:** Place 4 BP_LootLocker next to each other → only one is interactable. Works fine when isolated.
- **Expected:** Each locker should be independently interactable
- **Likely cause:** InteractionArea spheres (200 UU radius) overlap → priority/conflict issue. Moonville line trace may hit wrong actor, or multiple overlapping interaction volumes confuse the system.
- **Related:** General proximity-to-interaction issue — picking up weapons/items on the ground sometimes requires standing directly on top. Interaction button ignored or prompt doesn't show unless very close.
- **Systems:** InteractionSystem, BP_LootLocker, Moonville interaction layer

### BUG-003: Briefcases not sharing inventory across instances
- **Status:** OPEN
- **Severity:** Critical (core feature of TICKET-055)
- **Repro:** Place item in Briefcase #1 → walk to Briefcase #2 → open it → item not present
- **Expected:** All BP_Briefcase instances share one inventory via fixed UniqueID (DEADBEEF12345678ABCD0001CAFE0001)
- **Likely cause:** UniqueID not being set correctly on BP_InventoryComponent, or Moonville persistence not linking instances with same GUID within a single level (may only work cross-level on save/load)
- **Systems:** BP_Briefcase, Moonville persistence (BPI_InventorySaveActor), BP_InventoryComponent UniqueID
