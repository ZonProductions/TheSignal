# SIGNAL — Level Spec: Building 1, Floor 3

*Vertical-slice / demo level. Discussion notes — companion to `signal_story_bible.md`.
Placement: Chapter ~3 in the progression (office layer, after Water Treatment).
This is mid-game content presented as a standalone demo slice, NOT Chapter 1.*

---

## Why this is the demo level (even though it's Chapter ~3)

- **Offices are legible.** Players instantly read the space, so the demo showcases the
  core loop (interference tracking → search → unlock → fight fodder) without environmental confusion.
- **Mid-game = representative.** Marcus's kit and the enemy roster are at a real state,
  not the bare tutorial state. The demo shows the game, not the prologue.
- **Cold-open framing.** "Janitor, building's gone to hell, reach the tram." Stands alone,
  no Chapter 1 baggage required.

---

## Why Marcus is here

- **He's chasing the tram.** Out of the Water Treatment facility, the guide (over the failing
  earbud) says the campus is in hard lockdown and the only route toward the research labs —
  or out — is the inter-zone tram, and the nearest live platform is inside/atop Building 1.
- **Why 3F specifically:** he doesn't enter via the lobby. The water-treatment service
  corridor / utility skybridge dumps him out mid-building on 3F. A janitor arriving through
  the *guts* of the building, not the front door — true to the character.
- **Why it's a slog:** Building 1 slammed into automated **breach-protocol lockdown**.
  Keycard doors sealed, HVAC dampers shut, elevators dead. The safety system built to
  contain the void is now the thing trapping him in with it.

---

## The keystone: clearance asymmetry

A new janitor has **broad physical/maintenance access, near-zero security/data clearance.**
He can badge into mechanical rooms, custodial closets, and the vent network (his job), but
he **cannot** open security-tiered office or lab doors.

> Marcus can't open the manager's door because it's a *security* door and he's a janitor.
> So he goes around it — through the **vents**, which are his turf. The whole floor is
> "the maintenance guy cheating a building that was never meant to let him in."

This also drives the building-wide credential ladder: the **tram control on 5F needs a
security tier Marcus will never be issued**, so he physically steals his way up through
cards left on the desks/corpses of people who *did* have clearance.
**L4 card on 3F → L5 on 4F → tram control on 5F.** Each floor = steal the next tier.

---

## Floor 3 dependency graph

**Floor objective:** get the **Level 4 card** (in the Manager's corner office) →
it releases the breach-sealed **stairwell bulkhead to 4F**.

```
MANAGER'S OFFICE  (L4 card inside)
   └─ Door is SECURITY-sealed + lockdown-dead → janitor clearance can't touch it.
      Front door is OUT. Enter via the perimeter VENT instead.
         │
         ├─ (A)  Grille is SCREWED shut → need the cordless driver
         │        └─ Driver lives in your missing coworker's maintenance cart,
         │           far corner of the cube farm, parked at an ACTIVE VOID BREACH
         │           (spawn nest, enemy-dense, interference screaming).
         │           Lore: coworker was mid-way through sealing vents under
         │           breach protocol when he was taken. You finish his job.
         │
         └─ (B)  Vent must be PASSABLE → cut the HVAC supply fan
                  └─ Done at the Tech / mechanical room. While the fan runs,
                     the vent chase is pressurized AND it's the spawn highway
                     (void-spawn travels room-to-room through ducts — you hear it
                     on the tracker). Cutting power makes the crawl survivable AND
                     freezes the highway. The room also has the LOCKDOWN PANEL
                     (exposition: why every door sealed, and that it was on purpose).

   →  Crawl the vent → drop into Manager's office → grab L4 card.
      Lore beat: manager's last emails = the "Polaris knew" breadcrumb;
      a desk photo makes the corrupted-manager enemy you fight here tragic.
   →  L4 releases the stairwell bulkhead → 4F.  (Demo ends on the bulkhead
      grinding open / first glimpse up the shaft toward the tram. Stinger.)
```

(A) and (B) can be done in **either order** — light metroidvania non-linearity with no
real branch to author.

---

## Suggested floorplan

- **Skybridge entry** (from Water Treatment) → service corridor / break area, 3F east.
- **Open-plan cube farm** (central) — fodder at desks; far corner holds the void breach + cart.
- **Perimeter private offices** — including the **Manager's corner office** (L4 card).
- **Conference room** — search/resource, optional lore.
- **Tech / IT / mechanical room** — HVAC fan control + lockdown panel; interference-tracker spikes.
- **Stairwell** — bulkhead-sealed to 4F; releases on L4.

---

## Enemy-AI integration notes

Three distinct AI jobs on this floor (the tonal range the bible wants):

- **Vent spawn (non-humanoid):** ducts are AI highways. Fan ON = spawn migrates between
  rooms, tracked by interference; fan OFF = highway frozen. Systemic, not scripted — the
  player's puzzle solution (kill the fan) directly reshapes enemy movement.
- **Breach nest (cube farm):** persistent pressure source at the driver location. Lean into
  helplessness — likely *can't* be closed; you survive it and leave. Optional: let the player
  choke it by sealing that room's grilles (rewards the vent mechanic again).
- **Humanoid fodder (former staff):** at desks, in the break room — the "that used to be a
  person" beat. The corrupted **manager** is the personal capstone fight, recontextualized
  by the photo/emails you just read.

---

## Why the screwdriver isn't arbitrary

Grilles are screwed shut because **maintenance — Marcus's own department — had started
sealing vents to slow airborne spread** before they were overrun. The tool hunt is also a
character thread: you're following a dead coworker's unfinished task. Ties the vents to the
infection-as-airborne-signal idea without contrivance.

---

## Interference-tracker levers parked on this floor

- Tech/server room interference spike: option to make the tracker read *infrastructure*, not only
  enemy proximity. Lever, not locked.
- Vent crawl while fan runs = audio dread peak (you can't tell spawn distance over the roar).
