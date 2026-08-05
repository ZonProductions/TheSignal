# ResearchFacility — Asset Plan & Room-Dressing Guide

**Map:** `/Game/Campaign/ResearchFacility` (the dev map / EditorStartupMap — NOT the
`/Game/ResearchMegaPack/.../ResearchFacility.umap` pack demo).
**Written:** 2026-07-22, from a full audit of every owned Content pack + a live census of the level
(2815 actors, 234 unique placed meshes) + the SignalSTR story bible + the project checkpoint KB.
**Scope:** Floor 2 = half **admin**, half **research + medical**; plus the story-load-bearing
**Sub-Floor 3** (Ch.1 shielded room) and **Floor 1** (fuse-recovery). Keyed to your own room list in
`Docs/Research & Labs.MD`.

> **Bottom line up front.** Floor 2 is *furnished but pristine* — the census shows 700+ props already
> placed (34 desk sets, 48 chairs, fitted restrooms, 8 gurneys, 2 MRI, cafeteria). The real remaining
> work is (a) **completing the un-built named rooms**, (b) the **horror overlay** (nothing is
> overturned, bloodied, barricaded, or corrupted yet), and (c) **a very short genuine-gap list** — you
> own far more than you're missing. The whole facility can be dressed almost entirely from owned packs.
> Everything player-facing text stays a data field per project convention — this doc never bakes labels.

---

## 1. Do these FIRST — level-correctness fixes (independent of dressing)

These are defects the census surfaced. Fix before/around dressing; they are not room work.

| # | Issue | Fix |
|---|-------|-----|
| 1 | **1 StaticMeshActor at z=3121 has NO mesh assigned** | Assign the intended mesh or delete the actor. |
| 2 | **`ZP_MapPickup` placed with NO `ZP_MapVolume`** — the floor-2 map cannot render | Place a `ZP_MapVolume`; its `AZP_AreaID` MUST equal the MapPickup's AreaID AND the `generate_floor_plan.py` `AREA_ID` (locked rule). Center Z at player height (~3030). Then run the floor-plan generator. |
| 3 | **39 raw `/Engine` blockout Cubes still visible on Floor 1** (z 2000-2506) | Replace/remove — they read as unfinished graybox. |
| 4 | **SM_Floor_4 snap knots** (4 left for dev per 2026-07-12 checkpoint) | Finish the snap pass (double-cover, duplicate, short row, under-corridor tile). |

Also standing housekeeping from the KB (not blocking): strip log spam (`[ShamAnim]`, `[LatchProbe]`,
`[SignalSense] EVAL`, `[Scytheer] TICK`); the `UE5_ABP_IK_Pose "Accessed None"` per-frame BP exception.

---

## 2. What you own — pack-to-room ownership map

Environment/prop/FX/audio packs only (character/anim/weapon/system/UI packs excluded). Verdicts are
from the audit; asset names elsewhere in this doc are verbatim from pack listings.

| Pack | Assets | Best at | Weak/absent |
|------|-------:|---------|-------------|
| **ResearchMegaPack** | 3753 | THE core shell + medical ward + lab set-pieces (beds, gurney, MRI, capsules, biomass, control room, elevator, monitors) + **505-instance DecalPack** grime library | office small-props, benchtop lab gear, audio |
| **office_BigCompanyArchViz** | 1125 | Admin workhorse: bullpens, exec/manager offices, conference, kitchen, restrooms, security office, CCTV/biometric/safe, HVAC/duct layer, elevator | real lab/medical gear (only `SM_FirstAid`) |
| **Office** | 520 | Admin fast-fill: 16 pre-dressed desk-cluster prefabs, conference, records (file cabinets + 25-pc document clutter), restroom | medical/lab/security/industrial |
| **LEVELS/ModSci** ecosystem (ModSciInteriors 376 / ModSci_Engineer 273 / **ModSci_EngiProps 302** / ModularSciFi 615 / ScifiInteriorPack 248) | **1814** | Utility/plant/server/containment/corridors: `SM_WVD_Server`+`SM_StorageAreaNetwork`, gas cylinders (`SM_OxygenTank`,`SM_NitrogenTank_Covered`), **full fire-safety set**, hazmat sign decals, **lock-state glass doors**, ceiling kits, air ducts, glass observation floor, padded-cell walls | wet-lab benchtop gear, medical beds, office furniture (1 desk/2 chairs total) |
| **TreatmentStation** | 335 | Industrial interior: mechanical/plant/generator rooms, HVAC, pipe runs, catwalks, caged storage; 3 Reactor vessels pass as process equipment | office/medical/lab, readable signage, audio |
| **Marlitia_Outpost** | 786 (~half unique) | Clean-industrial modular kit + `M_Glitch` haunted-screen material, lockers, electrical boxes, fans, `SM_WallGrid` cage fronts | office furniture, medical/lab, paper clutter |
| **JunkerTown** | 1401 (~400 real) | Rusty derelict/maintenance props: tools, fuses, cable bunches, lockers, bins, `BP_TV`/`BP_monitorPack`, first-aid | too grungy for clean labs; no clean office/lab gear |
| **MilitaryAirport** | 249 | Security/perimeter/checkpoint + crate/barrel/container logistics (exterior scale) | interior office/lab/medical |
| **Biomass_Bundle** | 251 | **The flagship horror pack for this level**: modular creep, pulsating lifeforms, egg setpiece, whip-monster hazard, emissive growth | no clean-to-creep transition decals; no audio |
| **Blood_VFX_Pack** | 263 | Combat gore + static aftermath (`BP_Decal`, ground/artery splash, bleeding-ribbon drag trails) — already wired to `UZP_BloodFXComponent` | one decal master (no handprint/smear variety); no corpse meshes |
| **UWC_Bullet_Holes** | 563 | 170 surface-matched damage decals (sheetrock/wood/glass/ceramic/metal) + melee/scratch/crack/burn + POM blast craters | props/furniture/blood |
| **VFXSparkPack** | 43 | Damaged-electrical layer: arcing beams, `VFX_Electrical_explosion_nolight` (Lumen-safe), embers, `LF_FlickLight`, matched spark SFX | steam/water/fire |
| **HorrorLight** | 13 | Project's atmosphere kit: `BP_HorrorLight` (flicker), `NS_VoidMote`/`NS_SporeMote`/`NS_DustMote`, void materials, `SM_VoidCube` | fixture housing meshes, furniture |
| **BackgroundBuildings** + **PN_interactiveSpruceForest** | 101 + 67 | **Window scenery** — city skyline + dark treeline for the remote PNW campus outside every window | interior (none) |
| **Battery** | 10 | Emissive glow MIs (`GREENFLICKER1`,`REDFLICKER`,`GlowBlue`,`GlowRed`) = fake every status LED/alarm | meshes |
| **CharacterCustomizer** | — | `CCMH_Body_Male/Female` + `ApparelPack_Character` → **posed static corpses** via the bake pipeline (see §7) | — |
| Accent/stub packs | — | Container_Yard (chainlink+tarp+dirty ground), IndustrialArea (brick wall MI), Underground (emissive/grunge MIs), StarterContent/office_StarterContent (FX + surfaces + light BPs + ambience loops), LightProfilePack (2 IES), Laser/LaserSystem (security tripwires) | mostly materials/FX only |
| **Audio** (project bank 349 + Sounds 31 + gasp_Audio 294) | 674 | Doors/elevator, power events, creature voices, weapon impacts, player foley (concrete only), `MS_Signal_InterferenceBed` | **ZERO room tones / PA / medical-equipment loops / tile foley — the project-wide hole** |

---

## 3. The actual gap — one screen

After folding in the ModSci ecosystem, the genuinely-missing list is short. Everything else is
repurpose/kitbash from owned assets.

### Tier A — worth acquiring or generating (each unlocks multiple rooms)
| Gap | Why it can't be faked | Cheapest fill |
|-----|----------------------|---------------|
| **Benchtop wet-lab equipment** (fume hood, biosafety cabinet, glassware, microscope, centrifuge, incubator) | ZERO owned across all 30+ packs; capsules/monitors carry the "science" load but a bench lab reads wrong | ONE Fab "laboratory equipment pack" (search: *lab fume hood biosafety cabinet microscope centrifuge glassware*) — unlocks wet lab, tissue-culture, imaging, autoclave in one buy. Hero silhouettes that miss → meshy/tripo generate |
| **Morgue body-drawer bank** | No owned mesh reads as a wall of refrigerated drawers; it's the top medical-wing horror prop (drawers = jump-scare containers + reveal device) | Fab (*morgue body drawer cold storage cabinet*) or meshy-tripo |
| **Overhead surgical light dome** | Defines the treatment/surgery room from the doorway; nothing owned approximates it | Fab (*surgical operating light dome ceiling*) or meshy-tripo |
| **IVC ventilated cage racks** (vivarium) | `SM_WallGrid` only half-sells the stacked-cage silhouette | Fab (*ventilated cage rack vivarium*) or meshy-tripo |
| **Interior room-tone / PA / drip audio** | The audio-driven-atmosphere pillar has zero ambience beds; your #1 non-visual gap | **In-house** — your Ableton studio (stated strength) + `generate_sfx_elevenlabs` / `generate_voice_elevenlabs` via nwiro; route through `UZP_SFXStatics`. No purchase. |

### Tier B — generate in-house (no purchase)
- **Posed static corpses** — dress `CCMH_Body_*` in `ApparelPack_Character`, pose in PIE, bake via
  `UZP_MeshBakeUtils::BakeActorsToStaticMesh` (the proven `SM_Crawler_Ref` pipeline) → cheap
  `SM_Corpse_*` to scatter. 2 AM skeleton crew ⇒ sparse, individual bodies.
- **Plastic quarantine sheeting** — meshy-tripo draped tarp x3 + `M_Simple_Translucent`-derived MI
  (fallback: warped Plane primitives). One justified Fab buy only if generation fails.
- **Loose single-sheet paper scatter** — Plane primitives + 3-4 page MIs in one ISM BP.
- **Body bags / sheet-covered bodies** — meshy-tripo, or cloth-drape a baked corpse and re-bake.
- **Clean-to-creep transition decals** — Biomass_Bundle's documented weakness; derive a soft-falloff
  decal MI from `T_2k_Creep1_Albedo` masked by `T_Grunge_Decal_M` (interim: hide seams with UWC
  crack + blood decals).
- **Screen content** — "still logged in" desk monitors, frozen presentation slide, the anatomically-wrong
  scan, live-CCTV feeds (screenshot the real rooms via nwiro `take_screenshot` → import as MI), corrupted
  screens via `M_Glitch` (Marlitia).

### Tier C — repurpose owned (was previously mis-flagged as "buy")
These were "buy on Fab" in the raw wing analysis but the ModSci audit proved you already own them:
- **Server racks** → `SM_WVD_Server` (ModSciInteriors) + `SM_StorageAreaNetwork` (ModSci_Engineer).
- **Cryo / gas cylinders** → `SM_OxygenTank`/`SM_OxygenTank_B`/`SM_NitrogenTank_Covered` (ModSci_EngiProps).
- **Fire/safety props** → `SM_FireExtinguisher`(+Mount), `SM_FireAlarmBell`, `SM_FirePullStation`,
  `SM_EmergencyButton`, `SM_AlarmLight`, `SM_SafetySwitch` (ModSci_EngiProps).
- **Hazmat / warning signage** → `MI_HighVoltage`, `MI_DangerNitrogen`, `MI_Caution`, `MI_DiamondLabel`,
  `MI_GlovesSign`, `MI_NitrogenCylinders` (ModSci decal set) — no author-from-scratch needed.
- **Lock-state lab/containment doors** → `SM_Door_Small_A_Glass` + `MI_Door_Locked`/`MI_Door_Unlocked`
  + `Door_A_BP`/`Door_A_Close_BP` — reparent to your `BP_InteractDoor` for readable locked/unlocked.
- **PA/intercom physical anchor** → `SM_VOIPPhone` (ModSci_EngiProps) + `SM_speaker_01/02`,`SM_Megafon_1`.
- **Medical-kit prop** → `SM_Toolbox` with `MI_MedicBox`/`MI_MedicBox_b` reskin.
- **Padded isolation-cell walls** → `SM_Wall_Puffy_A` + `MI_Wall_Puffy` variants.
- **Ceiling interest + air ducts** → `SM_Ceiling_Main` (8 variants incl. integrated-pipe) + `SM_AirDuct_*`.

**Net purchases recommended: 1-2** (a lab-equipment pack; optionally a morgue/medical-props pack).
Everything else is owned, repurposed, or generated in-house.

---

## 4. Research + Medical wing (research half of Floor 2)

Keyed to `Docs/Research & Labs.MD`. Format: **Room — purpose · HAVE (owned) · NEED (gap → fill)**.
Story frame: Polaris biotech, "contained not created"; the plantmass void-spawn originates from
Polaris's own cultures/specimens — so the research half is the infection epicenter.

**Highest-ROI owned rooms (near-done today):**

- **Specimen / stasis-capsule bank** *(the single strongest owned-asset room)* — HAVE: `SM_Capsule_1/2`,
  `BP_Capsule_1`, `SM_Capsule_Ceiling_1`, `SM_Capsule_Pipe_1/2`, `SM_Biomass_1/2/3`, `SKM_Egg_Lifeform`,
  `SK_MegaspikanLarvae` (escaped specimen), `SM_Whip_Monster_Sleeping/Dead_*`, `SM_Control_Panel_1/2`,
  Battery LEDs, UWC cracked-glass. NEED: specimen jars (Fab/meshy), suspended fluid (translucent MI +
  tinted `NS_VoidMote`). *Horror: one capsule burst OUTWARD (larva escaped), one half-formed thing, rest
  dark; per-capsule LEDs failing red.*
- **Tissue-culture / clean grow-room (overgrown)** *(canon plantmass origin — sterile-white → void-choked)*
  — HAVE: capsules as culture chambers, full Biomass creep/mitochondria/mushroom set, `MI_Floor_White_1`,
  `NS_SporeMote`. NEED: laminar-flow biosafety cabinet (Fab lab pack), CO2 incubator (reskin
  `SM_ClosetMetal`+dial+`GREENFLICKER1`), clean-to-creep transition decals (§3). *Dose the infection
  heaviest here; taper toward the wet lab.*
- **BSL-3 containment suite + gowning airlock** *(echoes the shielded room; can seal the player in)* —
  HAVE: **ModSci lock-state glass doors** + `SM_WVD_Gate` bulkhead, `BP_Door_Lamp_01`/`BP_Wall_Lamp_80_red`
  status lamps, `SM_control_panel_01/02`, `MI_stripes_red`/`MI_cross_red`, PPE lockers (`SM_SM_Locker_01`)
  + hung `SM_Cloth_1`, Biomass breach growth, `SFX_Signal_Alarm`, UWC spidered security-glass. NEED: Class
  III biosafety cabinet/glovebox (the breach centerpiece — Fab/meshy), PAPR hood (Fab). *Interlocked doors
  trap the player with the threat; one suit missing from the bench = "who's still in there."*
- **Vivarium / animal housing** — HAVE: `SM_WallGrid_*` cage fronts, `SM_Fence_Grill`, `SK_MegaspikanLarvae`,
  Biomass burst growth, UWC scratch/bent-metal, `SM_Bottle` water bottles, lurk SFX. NEED: **IVC cage racks**
  (§3 Tier A). *Rows of empty cages with bent bars + scratch trails unsettle with no enemy; one cage burst
  from inside.*

**Rooms needing the lab-equipment buy to fully sell:**
- **Open wet lab / bench lab** — HAVE casework (`SM_Cabinet_*`), stools, monitors, sink stand-ins,
  Biomass foothold, `SM_Florosent`+`BP_HorrorLight`. NEED: fume hood, glassware, centrifuge, eyewash
  (Fab lab pack) + black epoxy bench tops (box primitive + `M_Black_1`).
- **Imaging & microscopy suite** — HAVE: `SM_MRI_1` anchor, monitor wall, `MI_Monitor_Screen_1`,
  `M_Glitch`, blackout curtains, Battery LEDs. NEED: the "anatomically-wrong scan" texture (generate),
  EM/confocal + isolation table. *Room dark but for monitor glow; one screen frozen on too many structures.*
- **Autoclave / sterilization + glass-wash** — HAVE: TreatmentStation Reactors as steam chambers, pipe/valve
  runs, `P_Steam_Lit`, wash sinks, drying racks, `SM_Thermometer_1`. NEED: gasketed double-door autoclave
  (reskin Reactor + door decal). *Body-sized chamber door ajar, steam pouring, a drag-smear leading to it.*

**Rooms fully coverable from owned (ModSci does the heavy lifting):**
- **Cold storage / -80 freezer farm** — `SM_Frigde`/`SM_ClosetMetal` banks, `BP_Metal_Door_1` walk-in door,
  `P_Fog`+`P_Steam_Lit` vapor, `SM_NitrogenTank_Covered` dewars, Biomass frozen growth. *Heavy latch = lock-in
  trap; frost-glazed glass with a body-shape behind it.*
- **Chemical & gas storage** — TreatmentStation tanks/drums, `SM_OxygenTank`/`SM_NitrogenTank_Covered`,
  `SM_WallGrid` cylinder cage, ModSci `MI_DangerNitrogen`/`MI_DiamondLabel` decals, VFXSpark ignition,
  `MI_Leaking_*`. *Hissing gas leak to cross; a shootable cylinder as improvised weapon.*
- **Data / server & compute room** — **`SM_WVD_Server` + `SM_StorageAreaNetwork` rows** (owned!),
  ModSci `MI_FloorPadding` raised floor, `SM_Fan_A`/CRAC units, Battery LED field, `SM_Terminal_1/2` save
  terminal, `MS_Signal_InterferenceBed`, VFXSpark shorted rack. *Loud/cold/red-lit save nook where fan hum
  masks approach; the signal originates from Polaris data systems.*

---

## 5. Medical wing (medical half of Floor 2) + utility rooms

Frame: Polaris's on-site **occupational clinic** that treated the first void-exposed staff (grounds the
morgue overflow + patient-zero isolation against the "miracle medicine" front). ~70% dressable from owned
today; much is **concentrating scattered props into named rooms**, not buying.

**Medical (already-placed props to concentrate):**
- **Clinic reception / intake** — `SM_Desk_1`, glazed check-in partition, waiting chairs/sofa, `SM_FirstAid`,
  `MI_cross_red`, stopped `Sm_Clock`. NEED (low): folded wheelchair (Fab/meshy), chart rack (repurpose shelf).
- **Exam room (×several down a curtained row)** — `SM_Bed_1`(+bare-mattress variant), `SM_Overbed_Table`,
  `SM_Stool_1`, `SM_Othoscope_1`, `SM_Vital_Monitor_1/2`, `SM_MedicalCurtain_1/2`, `SM_Iv_1`, UWC + Blood
  struggle decals. NEED: glove/sharps dispenser (one Fab buy covers every medical room), paper-roll exam
  table (reskin `SM_Bed_1`). *Curtain half-closed hides a slumped shape.*
- **Treatment / procedure (emergency)** — `SM_Gurney_1`/`BP_Gurney_1`, `SM_EKG_Monitor_1`, `SM_Vital_Monitor_3`
  (flatlined), still-dripping `SM_Iv_1`, Blood arterial-spray + `P_Bleeding_Ribbon` drag trail. NEED
  **(critical): surgical light dome** (§3), crash cart (kitbash `SM_Computer_Cart_1` + Fab/meshy defib).
  *Flatline beep, IV still dripping, drag mark leaving toward the door.*
- **Isolation / observation cell (Patient Zero)** *(horror centerpiece #1; the "Polaris knew" clue made
  physical)* — `SM_Bed_2`, `SM_Wall_10_Window` viewport, `SM_Glass_Separator_1` anteroom, `BP_Metal_Door_1`
  interlock (or ModSci lock-state door), `SM_Vital_Monitor_1` still tracing a pulse, `MI_stripes_red`/`cross_red`,
  `SM_camera_wall_01`, UWC spidered glass, Biomass `SKM_Big1_Ground_Pulsating_Lifeform` breathing where the
  patient lay. NEED: bed restraint straps (primitives — EMPTY straps are the scare), hazmat suits on a
  gowning bench (consistency-checks the Ch.5 rift-suit twist), neg-pressure gauge (`SM_Panel_1`+decal).
- **Pharmacy / dispensary** — `SM_Shelf_1/2`, `SM_Cabinet_5_A`, `SM_SafeSmall/Big` (pried-open loot gate),
  biologics fridge, `SM_Rollershutter` (half-down), `BP_LootLocker` backing. NEED: spilled pill clutter
  (Fab/meshy — cheap desperation beat), Pyxis dispenser (reskin `SM_Vending_Machine_1`).
- **Medical records** *(primary lore room)* — `SM_FileDrawerOpen/Close`, `Sm_FileCabinet`, document clutter,
  `SM_Desk_1`+`SM_Monitor_1`, `BP_ItemPickup` for readable finds. NEED: color-tabbed charts (author
  FText/DataAsset content — a chart flagged ISOLATED with a coworker's name). *A drawer open at a name;
  Tier-4 memos that "get bigger and worse."*
- **Morgue / cold body storage** *(horror centerpiece #2 — THE room)* — `SM_Gurney_1` (sheeted shapes),
  `SM_MRI_Bed_1` (autopsy-table stand-in), `SM_Capsule_1/2` (body-in-glass), `SM_Military_2` (corpse on slab),
  Biomass growth from stored biomass, Blood pooling to a drain, `P_Fog`+`NS_SporeMote`, `SFX_LARGE_SLIDE_DOOR`.
  NEED **(critical): refrigerated body-drawer bank** (§3 — top buy), autopsy downdraft table (reskin
  `SM_MRI_Bed_1`), body bags/sheeted overflow. *One drawer ajar and empty; overflow bodies on gurneys = ran
  out of room.*
- **Decontamination / emergency shower** — `Sm_Stall01`, `SM_Shower`, floor-drain grate, tile MI,
  `MI_stripes_red`, blood-to-drain, PPE doffing locker. NEED: deluge safety shower + pull chain (Fab/meshy —
  the silhouette that sells it).

**Utility / infrastructure (~90% owned — buy nothing):**
- **Electrical room / main distribution (fuse-box puzzle — RESEARCH1 already authored)** — TreatmentStation
  + Marlitia breaker panels, `SM_ElectricalEnclosure`, `SM_Fuse1-4` (the 3-fuse objective), Fab `electronic_fuse`
  seated pickup, VFXSpark arcing (`VFX_Electrical_explosion_nolight` keeps Lumen dark), `C1_POWER_OUT` +
  `C1_FUSE_BOX_COMPLETE` audio, `BP_ObjectiveContainer` backing. **This is a DRESSING pass — gameplay is done.**
  Throw the breaker and the lights AND the threat rise together. NEED (low): Arc-Flash signage (ModSci decals).
- **HVAC / mechanical (MEP)** *(over-covered — buy nothing)* — TreatmentStation + Marlitia + office_BigCompany
  air handlers/chillers/fans/ducts/pipes+valves, `BP_Fan_A_Light` (strobing-shadow gag, drop-in), catwalk
  grating, ModSci vents (creature ingress), `P_Steam_Lit`, `MI_Leaking_*`. *Fan behind a grate throwing
  strobing shadows; a vent hanging open.*
- **Maintenance / custodial closet + workshop** *(Marcus's own world)* — JunkerTown tools (ALREADY scattered
  on floor 2 — gather them here), `SM_ToolShelf`, `bolt_cutter_low_poly` (chained-door interactable), cleaning
  set, `SM_HotelKey` as the **janitor key ring** (progression item, no new art). NEED: mop + wringer bucket
  (meshy — the one prop that says "janitor," and Marcus IS the janitor).
- **Storage & supply (general + caged)** — shelving/racks, crates (Marlitia + MilitaryAirport pre-stacks),
  barrels, pallets, `SM_WallGrid`/`SM_Chainlink_Fence_3m` cages, `BP_LootLocker`. *Cage padlock, crate cover,
  a toppled shelf forcing a detour.*
- **Server & comms closet (IDF)** — ModSci `SM_WVD_Server`, `SM_Terminal_1/2`, cable runs, Battery LED field,
  `SM_Fan_A`, `M_Glitch` screen, `MS_Signal_InterferenceBed`, `BP_SavePoint`. *A save point that never feels safe.*
- **Service receiving / freight staging** *(verify with dev — floor 2 is upper, so freight-lift not ground dock)*
  — `SM_Rollershutter`, forklift, containers, crate stacks, `SM_FloodLight_1` (one hard shadow). Lowest priority.

---

## 6. Admin wing + shared building spaces (admin half of Floor 2)

Best-covered zone in the project — office_BigCompanyArchViz + Office dress ~90%, and the census shows the
heavy furniture already placed. Remaining work is **completing rooms + staging vignettes**, not sourcing.

**Rooms (owned coverage strong; work is staging):**
- **Elevator lobby / arrival atrium** — elevator kit (placed + wired), waiting cluster, glass frontage,
  `BP_GlassDoors1`, `SM_BiometricTime` badge reader, CCTV, stopped clock at 2:00. NEED **(critical): backlit
  Polaris logo/brand wall** (primitives + emissive MI + generated wordmark — build once, reuse), directory
  board, EXIT sign kit.
- **Admin reception** — `SM_ConferenceSecretaryRoom` desk, visitor seating, `SM_PhoneOffice` + `SFX_Phone_Ring`→
  `SFX_Phone_Silence` (owned zero-cost dread beat). Reuse the logo backdrop.
- **Open-plan bullpen** — partitions + 34 desk sets + 48 chairs (placed), Office desk-cluster prefabs for
  unbuilt bays, document/personal clutter, `SM_Florosent`+`BP_HorrorLight`, `M_Glitch` corrupted monitor.
  *Dress ONE cubicle as the void-possessed coworker's desk (Biomass under it, blood on the chair) — the
  tragic-personal gut-punch.* NEED: "still logged in" screen MIs, framed family photos.
- **Private manager offices (×2-4)** — desk sets, bookcases, file drawers (open=ransacked), lamps, `Sm_NamePlate`
  variants, `Sm_Blinds` (slatted shadow with `BP_HorrorLight` behind). NEED: per-office lore docs (content, not
  mesh). *Lock one office from inside, key in the records room = teaches the backtrack loop.*
- **Executive corner office** — exec desk, `SM_SafeBig` (key/loot gate), book wall, `SM_Brifcase`, window drapes,
  **BackgroundBuildings + spruce night campus view**. NEED: facility scale model on a plinth (kitbash ModSci
  pieces — a model of the building you're trapped in), Tier-2 "proud science" docs. *Canon guardrail: pride
  here, never Tier-4 dread — the wrongness is that everything is perfectly in order.*
- **Boardroom / conference** — `Sm_ConfernceTable` (placed — the Shambler-wander room), long table, projector +
  screen, whiteboards, glass walls. NEED: frozen presentation slide MI (`M_Glitch` swap). *Light by projector
  beam alone; chairs shoved back, one knocked over.*
- **Records / archive** — file cabinets, ransacked-vs-intact drawers, shelf prefabs, 25-pc document clutter,
  `SM_Scanner`, `MI_stripes_red` restricted stripe. *Prime spot for the ONE personal-dread note (canon caps
  1-2 campaign-wide): a file naming Marcus's family in words no HR file could hold.*
- **Copy / print room** — dated + modern machine rows, paper stock, bins. *Scripted beat: the copier wakes and
  runs a job by itself at 2 AM — cheapest statement of "the void manipulates electronics."*
- **Break room / kitchenette** — coffee/espresso, microwaves (placed), fridge, vending, `SM_WeekBoard` notices,
  `P_Steam_Lit` still-warm coffee. *Still-steaming coffee vs the stopped clock = the dread.*
- **IT / server closet** — ModSci `SM_WVD_Server` (owned — was flagged "buy"), breaker cabinet, cable spaghetti,
  `SM_Fuse1-4` (ties to RESEARCH1), Battery LEDs, VFXSpark arcing rack. NEED: rack/fan hum loop (generate).
  *Constant hum masks footsteps; when power state flips, sudden silence here is itself an event.*
- **Security office** *(hub/save room)* — security-room kit, `BP_monitorPack`/`BP_TV` CCTV wall, `M_Glitch`
  dead feeds, `SM_SafeSmall` key lockbox, `SM_BiometricTime`, weapon-rack dressing, the cameras themselves.
  NEED: live-CCTV feed MIs (screenshot real rooms → import). *One monitor shows movement in a room you just
  cleared; a dead contractor (`SM_Military_2`) at the desk = the wing's first body.*

**Shared circulation:**
- **Corridors** — shell placed (831 walls), `BP_HorrorLight` ×31, `SM_Florosent`, fire-safety dressing
  (ModSci `SM_FireExtinguisher`/`SM_FireAlarmBell`/`SM_FirePullStation`), `SM_speaker` PA anchors, ModSci
  vents, wayfinding decals, DecalPack grime, UWC scratch/crack, dust motes. NEED: EXIT sign kit, room-tone bed
  (generate). *One corridor fully dark forces the flashlight; a blood drag-trail crossing from medical INTO
  admin stitches the two halves; `NS_VoidMote` density rising toward the research seam = the no-UI spatial clock.*
- **Restrooms (built — enrichment only)** — 12 toilets/sinks/kiosks placed; add drip audio, cracked-tile UWC
  decals, one closed stall with blood pooling under it, mirror facing the entry (reflection dread, zero tricks).
- **Janitor closet** — cleaning set, chemical clutter, `SM_HotelKey` on a hook. NEED: mop+wringer bucket
  (meshy). *Marcus's shift checklist with his own name (authored doc) makes this the most personal room.*
- **Stairwell core** — stairs + `BP_Surface` cuts done, `SM_Railing_1` ×42, red emergency lights,
  `MI_numbers_*_red` floor stencils, exit-door language. *A bloody handprint on the rail pointing DOWN,
  against your upward objective; an elevator ding echoing that nobody called.*
- **Cafeteria / vending** — 24 cafe seats placed, vending bank (loot), abandoned meals, `SM_CafeNeon` (kill
  the emissive), closed serving shutter (dodges building a kitchen). *2 AM = 3-4 tables with cold meals, not
  a rush; big open volume = good Shambler patrol space.*
- **Locker / changing room** *(admin↔medical seam)* — locker banks (clean + rusted variants), center benches,
  coats/lab coats on hooks, showers beyond, mirror. *One locker open with a pickup; one rattles on script;
  natural spot for the first interference-tracker spike as you cross toward medical.*

---

## 7. Horror / narrative dressing overlay (whole floor)

Applied ON TOP of built rooms. Almost all placement work — the census confirms floor 2 is furnished but
pristine. Organize as 13 systems; stage ONE hero scene per zone, not uniform coverage.

1. **Blood & violence aftermath** — Blood_VFX_Pack `BP_Decal` + ground/wall/artery splash + `P_Bleeding_Ribbon`
   drag trails; UWC `MI_Flesh_1`/`MI_Knife_1`. NEED: handprint/smear/dried-brown variants (derive MIs from
   `M_BloodDecal_Master`). *A drag ribbon ending at a closed door beats ten random splats.*
2. **Corpses & remains** — CCMH bake pipeline (§3 Tier B), `SM_Military_2` dead guard, sheeted gurneys.
   Sparse + individual (2 AM). The possessed-coworker beat = named corpse-bake + nameplate + personal doc.
3. **Overturned furniture & evacuation scatter** — rotate/tip a third of the 48 placed chairs (zero new
   assets), document clutter, yanked file drawers, abandoned packing boxes. NEED: loose-paper ISM (§3).
4. **Barricades & forced entry** — cabinets/tables shoved against doors, JunkerTown boarding/broken panels,
   `SM_Tape_1` (29 placed — extend), `SM_Sandbags`. NEED: nailed-plank barricade BP (primitives +
   `M_Chipboard_1`), chain+padlock (meshy, pairs with owned bolt cutter). *Intact from your side = survivors
   fled inward; splintered from the far side + `MI_Axe` + blood = it got in.*
5. **Broken glass & impact damage** — UWC `MI_Glass_Common_1`/`MI_Glass_Bulletproof_1`/`SM_Mesh_For_Glass` +
   crack/blast decals, surface-matched to both halves. NEED: shard-scatter ISM, pre-broken pane variants
   (duplicate pack mesh, never edit original). *Glass tells trajectory — shards inside = something entered.*
6. **Biomass infection gradient** *(the level's pressure system — denser = worse whispers, no UI)* — full
   Biomass_Bundle creep/pulsating/egg/whip set + `NS_SporeMote`. NEED **(critical): clean-to-creep transition
   decals** (§3). Author as rings: emissive epicenter → creep runs → vein decals → single spore motes at the
   clean boundary. Sterile-white medical bay half-consumed = the pack's intended contrast.
7. **Void presence & reality distortion** — `NS_VoidMote`/`NS_SporeMote`/`NS_DustMote` graded haze, `SM_VoidCube`
   + void materials, `M_Glitch` screens, `MS_Signal_InterferenceBed`. NEED: whisper VO (generate, LOW dosage —
   a handful campaign-wide). **GOTCHA: `NS_VoidMote` MUST carry its interference tag** (2026-07-12 checkpoint —
   one lost it silently) and the viewport Show→Particles flag hides them in-editor while PIE shows them.
8. **Emergency & lockdown lighting** — `BP_Red_Light`, ModSci `BP_Wall_Lamp_80_red`/`MI_Emission_Common_Flicker`/
   `MI_Light_Off`, `BP_HorrorLight` (31 placed), Battery status LEDs, IES throw patterns, `C1_POWER_OUT`. NEED:
   EXIT sign kit, per-zone light-state controller (C++). *Lockdown red marks the clearance gradient — deeper =
   redder — mapping light to security tiers for free.* Respect KB: no per-level exposure retune, sealed rooms
   pitch black, fill lights shadowless.
9. **Electrical failure & sparks** *(best-covered system — pure placement)* — VFXSpark arcing/embers/
   `VFX_Electrical_explosion_nolight`, hanging severed cables, `SM_Fuse1-4`, scorch decals. *Stage sparks as
   threat-maskers: an arcing panel ahead pulls the eye and covers the Shambler's footstep behind.*
10. **Quarantine & containment dressing** *(carries the central "Polaris knew" clue — containment EXISTED
    pre-breach)* — `SM_MedicalCurtain`, `SM_Tape_1`, ModSci hazard stripes/cross, `SM_Tente_1/2` decon tents,
    tarp fence, sandbags. NEED **(critical): draped plastic sheeting** (§3), biohazard bags, PPE scatter +
    hanging hazmat suit (bake ApparelPack coveralls). *Two vintages: weathered pre-breach vs panicked night-of.*
11. **Found-narrative anchors** — document staging on desks/floors, whiteboards, `SM_Terminal_1/2` lore
    terminals, `M_Glitch` corrupted state, printed-email trays, `SM_Megafon_1`/`SM_VOIPPhone` PA source,
    `SM_HotelKey` badges (ties to `BP_CardReaderPanel`). NEED: tape-recorder/dictaphone (meshy), whiteboard
    scrawl textures, **readable content = FText/DataAsset (you author from SignalSTR)**. *Placement rule: Tier-2
    bravado in the open admin bullpen, oblique Tier-4 fragments only behind security doors.*
12. **Audio dressing** *(the audio-driven pillar — your #1 gap)* — `MS_Signal_InterferenceBed` backbone,
    fluorescent/steam/wind loops, `SFX_Phone_Ring`/`Silence`, alarm/power stingers, reverb submix chain,
    physical PA speakers. NEED **(critical): interior room-tone loops** (facility hum/vent/server whine),
    **PA/intercom content** (evac loop, one corrupted announcement), **drip loops**, **biomass squelch bed**,
    **tile footstep foley** (bank is concrete-only). All in-house (Ableton + ElevenLabs → `UZP_SFXStatics`).
    *Keep room tones low-mid so the interference tracker owns the highs; one authored dead-silent room amid
    humming corridors is a scare by contrast. `SA_EnemyVoice` is dead — never route through it.*
13. **Grime, decay & atmosphere wear (runs LAST)** — DecalPack 505 grime library, TreatmentStation leaks,
    puddles, `P_Fog`, dust motes, steam, `M_Concrete_Grime`/`M_Metal_Rust`, scorch. NEED: dropped ceiling
    tiles + debris (primitives), active ceiling-drip Niagara. *Facility is HOURS-abandoned — bias toward fresh
    damage (drips, airborne dust, new scorch) over years-derelict decay; DecalPack has both, pick deliberately.*

---

## 8. Story bookends — Sub-Floor 3 + Floor 1 (don't skip these)

Per SignalSTR the Research Facility is the **campaign bookend** (Ch.1 opens here, Ch.5 returns). The wing
plans above cover Floor 2; these two need their own passes.

- **Sub-Floor 3 — the shielded containment room** *(Ch.1 opening / tutorial; the single most narratively
  load-bearing space)* — explicitly small: *"a couple of rooms, 2 key objects, an elevator, and a fuse box."*
  Marcus (first-week night-shift janitor) is cleaning the shielded room when the 2 AM breach hits; the earbud
  interference tracker is born the moment he steps out. **The shielding itself is the clue — someone built
  containment here, so Polaris knew.** Dress from: ModSci shell (`SM_WVD_Gate` bulkhead, lock-state glass door,
  `SM_Wall_Puffy_A` shielded-room walls, hazard signage establishing shielding EARLY but obliquely) +
  ResearchMegaPack + owned decals + the janitor kit (mop bucket, cart, `SM_HotelKey`). Facility on aux power.
  2 key objects + fuse box + elevator are the gameplay; keep it tiny and readable — Marcus learns the building
  as the player does.
- **Floor 1 — fuse-recovery / research-industrial level** — census: only *moderately* dressed, with the 39
  raw engine Cubes (fix #3) and the full Biomass set at z 2017-2031. Demands *"light puzzles and maze-like
  navigation"* with shamblers/crawlers loose; the pipe→first-tools progression; the first humanoid fodder (the
  possessed coworker). Finish: remove/replace the blockout cubes; integrate the existing biomass into the §7
  gradient; dress the control room (already has panels/terminals) + pipe corridors from TreatmentStation/ModSci.
- **Ch.5 containment zone + maintenance shaft** — legitimately deferrable; call it out as out-of-scope-for-now
  rather than silently skip. Re-entry via the sewer-fed maintenance shaft (janitor logic: back routes);
  security levels 1-3 to the containment; the rift-suit twist stage. The protective suit + shielded room must
  consistency-check as one physics (void-air = an exposure you seal out or mask against).

---

## 9. Cross-cutting rules (apply to every room)

- **Performance — the level is draw-call / dynamic-shadow bound** (not GPU/tri). Repeated new props (chairs,
  desk-monitor sets, creep tiles, cage fronts, decal clusters, barricade planks) MUST be authored as
  **ISM/HISM or Packed Level Actors**, not loose actors. `UZP_RuntimeISMBatcher` is PIE-only, so the *editor
  viewport* still renders every loose actor — authoring must batch, don't rely on it. New lights MUST be
  **Stationary/Static** with shadow-casting capped; standardize on `BP_HorrorLight`; don't pile on more
  shadow-casting dynamic light BPs. Run `Scripts/fix_all_generators2.py` after each pack-mesh placement pass
  (fixes convex-hull invisible walls on purchased meshes).
- **Windows** — Floor 2 uses `SM_Wall_10_Window`/`SM_Wall_10_Glass` throughout; every uncovered window shows
  skybox/void. Place ONE consistent exterior backdrop ring (BackgroundBuildings skyline + spruce treeline for
  the remote PNW campus) behind ALL window walls, not per-room.
- **Ceilings** — currently flat consolidated `SM_Floor` mirror tiles. Add cheap vertical interest with ModSci
  `SM_Ceiling_Main` variants (incl. integrated-pipe) + `SM_AirDuct_*` + office_BigCompany duct kit + hung broken
  fixtures + drip Niagara — but **batch these too** (respect the consolidated HISM ceiling + draw-call budget).
- **Art-style consistency — one shell family per wing.** Don't randomly mix: admin = office realism
  (office_BigCompany + Office); research/utility = pick ONE of ModSci OR ResearchMegaPack per room and stay in
  it; keep TreatmentStation reactor-scale vessels reconciled. ScifiInteriorPack blends least — use sparingly.
  The dated Office kit reading as budget-starved back-office (records/copy/janitor) is fine and deliberate.
- **Lock & key progression** — 54 `BP_InteractDoor` + `BP_CardReaderPanel` + `SM_HotelKey` exist. Build a
  one-page clearance-tier table: each locked door → its key/card item → the room it lives in, cross-checked so
  **no key sits behind its own lock**. Map lock state to the Tier-2→Tier-4 security ladder (ModSci lock-state
  glass doors give readable locked/unlocked language). *(This table is the one piece of gameplay design still
  to author — flagged here, not built.)*
- **Text is always data** — every room sign, floor name, objective title, document body, nameplate, and
  placard is an `FText`/DataAsset field with a neutral placeholder. This doc never bakes player-facing text;
  you author it from SignalSTR.
- **Asset safety** — never edit a pack original (duplicate first); never `save_asset` on something just
  restored/migrated; deprecate to `_DEPRECATED/` rather than delete.

---

## 10. Suggested build order

1. **Correctness pass** (§1) — map volume, empty actor, blockout cubes, floor snap. Unblocks the map + removes
   graybox.
2. **Complete the un-built named rooms**, wing by wing, from owned packs (§4-6). Research/medical half first
   (weakest currently), then finish admin staging.
3. **The two Tier-A generation/buys in parallel** (§3): kick off the lab-equipment Fab search + the in-house
   audio bed (Ableton/ElevenLabs) early — both gate multiple rooms and the audio pillar.
4. **Horror overlay** (§7), one hero scene per zone: blood → corpses (bake pipeline) → biomass gradient →
   quarantine sheeting → lockdown lighting → found-narrative anchors.
5. **Story bookends** (§8): Sub-Floor 3 shielded-room suite, Floor 1 finish.
6. **Grime/decay wear pass LAST** (§7.13) — glues the systems together and hides their seams.
7. **Cross-cutting cleanup** (§9): window backdrop ring, ceiling interest, lock&key table, perf batching audit.

---

### Sources
Live census (`Saved/asset_census/researchfacility_census.json`), full pack audit (17-agent workflow
`wf_6a2cd13b-bba` + ModSci follow-up, 1.9M tokens), `SignalSTR/` story bible, `Docs/Research & Labs.MD`,
`Docs/re2_rooms.csv`, and the ResearchFacility checkpoint history (2026-06-26 perf census through 2026-07-18
objective/perf passes). Checkpoint: `checkpoints/2026-07-22_researchfacility_asset_audit_workflow.md`.
