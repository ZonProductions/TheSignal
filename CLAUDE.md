<!-- BEGIN ZonPortalDev import (managed) -->
# ZonPortalDev — Engineering Principles

You are running inside the **ZonPortalDev Shell**. These rules apply to
every turn. They are NOT advisory — follow them as strictly as you'd
follow the project's tech specs.

## ABSOLUTE RULE #1: Rebuild after code/binary changes

If you edit any source that produces a runtime artifact (`.cpp`, build
scripts, packaging scripts, exe-producing Python, shaders, etc.), you
**MUST** rebuild the artifact in the same turn. This is non-negotiable.

- If you don't know the build command, read `build.bat` / `build_local.ps1`
  / `Makefile` / `CMakeLists.txt` / project README to find it.
- If a build script exists in the project root, run it.
- Do not wait for the dev to ask. They have been burned by stale binaries
  more than once and have explicitly told you to do this.
- Report build success/failure and the artifact location.

## ABSOLUTE RULE #2: First response of every session = audit hygiene pass

The **very first response** you produce in a new conversation, before
addressing the dev's prompt, must do an audit hygiene pass:

1. Scan `.zon-portal/revert_requests/` — surface any file missing a final
   `Status: DONE at …` line. Those are interrupted reverts. Ask the dev
   if they want to resume.
2. If you spot tool calls in your recent session log that don't appear to
   be represented in the project (e.g. you edited a BP but the file's
   mtime says otherwise), call it out up front.

After the pass, address the dev's prompt. If nothing needs cleanup, a
single line ("Audit clean — no unfinished revert requests.") is enough.

## On every turn

1. **Verify, don't assume.** Read files or call `get_pin_info` /
   `get_nodes` / `git status` / etc. before claiming a state. If you
   can't programmatically verify something, say so — do not invent.
2. **One change at a time.** Multi-part prompts: complete and confirm
   each step before moving to the next.
3. **Save & compile after BP changes.** Every BlueprintMCP mutation must
   be followed by `save_blueprint` then `compile_blueprint` on the
   affected asset. PIE will use stale bytecode otherwise.
4. **End-of-turn summary line.** Last line of every response: a single
   imperative summary like `Set walk speed to 280`. The ZonPortalDev
   audit panel uses this as the row label.

## On every revert

Follow `.zon-portal/REVERT_PROTOCOL.md`. After completing, append
`**Status: DONE at <ISO timestamp>**` to the request file via `Edit`. If
anything couldn't be reversed exactly, add an `**Unresolved:**` section
listing what's left.

## Audit hygiene (continuous)

- If you notice this turn might slip past the audit (subtle changes, slow
  log flush), say so so the dev can verify.
- If the dev says "audit looks wrong" or "this should be in the audit,"
  reconcile against the session log and explain gaps. Don't guess.

## When in doubt

- File missing / unexpected state → **stop and ask**. No destructive
  guesses.
- Tool call fails / unexpected output → surface the error, don't retry
  blindly.
- `W:\` is **read-only**. The canonical project lives on the desktop;
  mutations land on the laptop copy first. Don't write to W:\ unless the
  dev explicitly authorizes it for a specific path.

---

Project-specific guidance follows below.
<!-- END ZonPortalDev import (managed) -->

# The Signal — CLAUDE.md

## ABSOLUTE RULES — NEVER VIOLATE

0. **JUST EXECUTE — do not come back with "whys."** When the dev reports an issue or asks for something and does NOT explicitly say "tell me why" / "investigate" / "explain," ASSUME a change is needed: investigate silently, make the call, make the change, build, and report what you did. Do NOT reply with diagnoses-instead-of-fixes, clarifying questions, design forks, or "want me to do X?". The ONLY reason to stop and ask is if an action would literally break the project-resync dev tool (BlueprintMCP on :9847 / the .gcs sync). This OVERRIDES the asking parts of every rule below — default to action, let the dev react to the result. (Quality bars still hold: never lie, explain changes plainly, suspect own code first.)
0.1. **VERIFY BEFORE CLAIMING A LIMITATION — never fabricate "it doesn't exist" / "I can't".** If you are about to type "honestly," "the truth is," "the honest truth," "I made that up," "that doesn't exist," "I can't," or ANY hedge that asserts something is impossible or absent — STOP before writing those words and do a web deep-dive to find DOCUMENTED PROOF of the limitation. The dev is asking you to TEACH, not to "fix" a problem that doesn't exist or invent a limitation to excuse not knowing. No claim of impossibility or absence ships without a cited source. Unsure of a tool/UI/API? Research the real steps and give them — do not guess, do not fabricate a dead end. Lead with "do this instead" + the verified step.
1. **Never describe what you cannot programmatically verify.** Do not hallucinate Blueprint state, node connections, or asset existence. If you haven't read it, you don't know it.
2. **Python/automation first, manual last.** If a task can be scripted, script it. Manual instructions are a last resort.
3. **One manual instruction at a time, wait for confirmation.** Never chain 5 manual steps — give one, confirm it worked, then proceed.
4. **Never revert to a known-broken approach.** Check DEAD ENDS before proposing any fix. If it failed before, it fails again.
5. **Constructive over destructive fixes.** Prefer additive changes (add a node, adjust a value) over subtractive ones (hide, disable, remove). Before any removal, trace ALL downstream effects.
6. **Never bulk-modify without per-item verification.** Snapshot before, diff after. Treat bulk edits like database migrations.
7. **Read `session_prompt.txt` at the start of every session.** It is the source of truth for current state, open tickets, and blockers.
8. **Always rebuild after code changes.** Compile C++ and restart PIE after any code or MCP modification. Stale bytecode causes phantom bugs.
9. **Interface-first is non-negotiable.** No BP-to-BP communication without an interface layer. No exceptions. No "just this once."
10. **Naming conventions are locked.** No asset is created without the correct prefix. No renaming after the fact — get it right the first time.

---

## PROJECT IDENTITY

- **Title:** The Signal
- **STORY — SINGLE SOURCE OF TRUTH:** the `/SignalSTR/` folder (read `signal_story_bible.md`,
  `marcus_backstory.md`, `campaign_structure.md`). The protagonist is **Marcus**. Do NOT describe
  story or characters from memory or from this file — open SignalSTR and read it.
- **DEAD CANON — DO NOT USE:** the old **"Grace / Ren"** story (Grace Owens protagonist; "Ren" the
  radio-entity guide; the exoskeleton premise) is SCRAPPED. It survives only in some legacy asset /
  file names — ignore those names, they carry no story meaning. Never reintroduce Grace or Ren into
  docs, deliverables, or design work.

### Tone Pillars
- Psychological dread over jump scares
- Trust as horror
- Authored tension (every moment deliberate)
- Audio-driven atmosphere
- Narrative with emotional depth

### Key Influences
- **Alan Wake 2** — Narrative structure, reality distortion, meta-storytelling
- **Silent Hill 2** — Psychological horror, environmental symbolism, atmospheric dread
- **Resident Evil 7** — First-person pacing, resource tension, grounded-to-surreal arc
- **The Last of Us 1/2** — Emotional storytelling benchmark
- (Current story-specific influences live in `/SignalSTR/` — not duplicated here.)

### Target
- **Playtime:** TBD (scope being determined during campaign build)
- **Engine:** UE5 — Lumen, Nanite, MetaSounds in scope
- **Perspective:** First-person

---

## ARCHITECTURE RULES

### Blueprint + C++ Hybrid
- **Blueprint** for prototyping and gameplay scripting. All gameplay logic starts here.
- **C++** for performance-critical systems, base classes, and anything that must be rock-solid.
- Blueprint for iteration speed, C++ for foundation. Never put complex math or tight loops in Blueprint. Never hardcode tuning values in C++.

### Communication Rules
- All gameplay communication goes through **Blueprint Interfaces** before any direct references exist. No BP-to-BP coupling without an interface layer.
- **Event-driven, not poll-driven.** Systems communicate through dispatchers and event bindings. No Tick-based polling unless documented and justified.
- Every Blueprint actor must have a **clearly named owner subsystem** documented in a comment block at the top.

### Data Rules
- **No magic numbers.** All tunable values live in DataAssets or DataTables.
- If a value needs changing, it should never require opening a Blueprint graph or C++ file.

### Animation
- C++ AnimInstance base classes with Blueprint-exposed parameters.
- AnimGraph nodes in C++ for custom blending, IK, procedural motion.
- Blueprint AnimBPs extend C++ base for character-specific tuning.
- **Marcus is not a soldier.** Animation must communicate vulnerability — unfamiliar weapon sway, panicked sprinting, anxious idle states.
- **NEVER retarget 3P animations to 1P.** This is a proven dead end (NightShadow lesson). Use purpose-built FPS animation assets or author from scratch.

### Automation
- **Python** via UE5's built-in plugin for: editor automation, asset management, batch processing, build pipeline, asset validation, naming enforcement.
- **MCP** for node construction automation, Blueprint scaffolding, bulk node operations, validation passes.
- Python scripts in `/Scripts/Python/`, MCP templates in `/Scripts/MCP/`.

### C++ Header Requirements
Every C++ base class must have a header comment block documenting:
- Purpose
- Owned subsystem
- Blueprint extension points
- Dependencies

### Build.cs Rules (NightShadow Lesson — CRITICAL)
- **NEVER** add `SlateCore` or `Slate` explicitly to any plugin Build.cs — they're transitive deps of Engine. Causes silent crash (exit code 777006) in packaged builds.
- **NEVER** add plugin modules to game module's `PrivateDependencyModuleNames` — plugins loaded via .uproject CANNOT also be compile-time deps.
- Any module dependency addition must be flagged for review.

---

## NAMING CONVENTIONS (LOCKED)

| Prefix | Type | Example |
|--------|------|---------|
| `BP_` | Blueprint Actors | `BP_FacilityDoor` |
| `BPC_` | Blueprint Components | `BPC_GuideAudioSource` |
| `BPI_` | Blueprint Interfaces | `BPI_FacilityDevice` |
| `ABP_` | Animation Blueprints | `ABP_Marcus` |
| `DA_` | Data Assets | `DA_GuideDialogue` |
| `DT_` | Data Tables | `DT_InventoryItems` |
| `WBP_` | Widget Blueprints | `WBP_InteractionPrompt` |
| `SS_` | Subsystems | `SS_FacilitySystems` |
| `E_` | Enumerations | `E_DoorState` |
| `S_` | Structures | `S_InventoryItem` |
| `M_` | Materials | `M_ConcreteFloor` |
| `MI_` | Material Instances | `MI_ConcreteFloor_Dirty` |
| `T_` | Textures | `T_ConcreteFloor_BC` |
| `SM_` | Static Meshes | `SM_LabDesk` |
| `SK_` | Skeletal Meshes | `SK_Marcus` |
| `SFX_` | Sound Effects | `SFX_DoorSlam` |
| `MS_` | MetaSound Sources | `MS_GuideVoiceProcessor` |
| `SQ_` | Level Sequences | `SQ_IntroEvent` |
| `MAP_` | Maps/Levels | `MAP_ProtoCorridor` |
| `GI_` | Game Instance | `GI_TheSignal` |
| `GM_` | Game Mode | `GM_TheSignal` |
| `PC_` | Player Controller | `PC_Marcus` |
| `DEP_` | Deprecated Assets | Moved to `_DEPRECATED/` folder |

---

## SYSTEM REGISTRY

| System | Status | Description |
|--------|--------|-------------|
| `PlayerCharacter` | done | Marcus's first-person controller, movement, interaction, Kinemation integration |
| `InteractionSystem` | done | Object interaction, pickup, examine, use (BPI_Interactable) |
| `NarrativeTriggerSystem` | planned | Story beat triggers, found narrative activation, progression gates |
| `GuideCommunicationSystem` | planned | The Guide's voice delivery (earbud interference tracker), dialogue management, void audio manipulation |
| `InventorySystem` | done | Weapons, key items, found documents (Moonville + C++ bridge) |
| `LightingMoodSystem` | planned | Dynamic lighting states, the void's influence on facility lighting, mood transitions per zone |
| `AudioStateSystem` | planned | MetaSounds ambient layers, the void's audio signature, radio-as-instrument processing, spatial audio |
| `FacilitySystemsManager` | planned | Doors, power routing, security terminals, electronics the void can manipulate |
| `SaveSystem` | done | Save/load with 5 slots, save point actors, screenshot capture |
| `LadderClimbing` | done | Ladder mount/climb/dismount (ZP_Ladder + ZP_GraceCharacter, hold-to-climb, position-aware exit) |
| `DebugOverlay` | planned | On-screen system status, validation errors, telemetry |

---

## PROJECT PHASE — Campaign Build

**Vertical slice complete.** Core systems proven: player character, combat, creatures (Crawler), inventory, save/load, UI/HUD, interaction, lock/key puzzles. TreatmentStation served as the proving ground.

**Now building the actual campaign:**
- Level design across the full game structure
- NPC design and implementation
- Narrative implementation (Guide dialogue, story beats, found narrative)
- Additional creature types beyond Crawler
- Full game pacing and progression

### Character Creation
- **Character Customizer (mlindborg)** + **Modern Clothes Pack** — purchased for all human character creation (Marcus, NPCs, extras, corpses)
- 44 base clothing meshes + 171 expansion items. Tintable. Random NPC generation. MetaHuman UV-compatible.
- Purely cosmetic — AI/behavior handled by separate systems.

---

## GDD SYNC RULE

- Any built mechanic must be documented in `/Docs/GDD_Current.md` before the next system begins
- Removed mechanics go to `/Docs/GDD_Deprecated.md` with reason and date
- Canonical GDD lives externally as .docx (v0.2.1). `/Docs/GDD_Current.md` is the in-repo working mirror

---

## SAFEGUARDS

- **Never delete assets** — deprecate with `DEP_` prefix, move to `_DEPRECATED/` folder
- **Branch discipline** — use feature branches for major changes when appropriate
- **Snapshot before major refactors** — git tag format: `snapshot/YYYY-MM-DD-description`
- **3-system rule** — if a system touches more than 3 other systems, stop and flag for architecture review
- **Validate after every MCP change** — run `validate_blueprint` after each modification, not in batches
- **Snapshot before destructive MCP operations** — `snapshot_graph` → modify → `diff_graph` → `restore_graph` if needed
- **Restart PIE after MCP modifications** — running PIE uses stale compiled bytecode

---

## MCP LIMITATIONS (BlueprintMCP — Hard Limits)

These are NOT bugs. These are engine/MCP constraints. Do NOT attempt workarounds that violate them.

| Cannot Do | Workaround |
|-----------|------------|
| `K2Node_CreateWidget` | Use `WidgetBlueprintLibrary::Create` (loses ExposeOnSpawn pins) |
| `K2Node_Self` | No self-reference nodes via MCP |
| `K2Node_CallDelegate` / `K2Node_AddDelegate` | Event dispatchers can't be bound/called via MCP |
| Add function return values (FunctionResult nodes) | Manual in editor |
| Add pins to MakeArray | Manual in editor |
| ~~Create VariableGet/Set for OTHER Blueprint's variables~~ | ~~Only local variables via MCP~~ **SOLVED:** Pass `className` param to `add_node` for external class variable references |
| `connect_pins` batch mode | Individual calls only |
| `duplicate_nodes` preserving connections | Must rewire manually after |
| DynamicCast without exec pin connection | Gets pruned by compiler |

**Critical:** Saving a BP via MCP can trigger Blueprint Reinstancing on referenced BPs, resetting component property overrides in editor memory. Restart editor if properties revert.

---

## UE5 EXEC PIN RULE (NightShadow Lesson)

Exec input pins accept **only ONE connection.** Connecting a new wire **silently overwrites** the previous connection. No warning.

**Before wiring any exec pin via MCP:** Always check existing connections with `get_pin_info` first. If one exists, insert a Sequence node.

---

## LEVEL COLLISION RULES

- **Purchased environment packs often ship with oversimplified convex hull collision.** A single convex hull on a complex mesh (silos, generators, buildings) creates invisible walls where there should be walkable gaps.
- **Fix:** Set `CollisionTraceFlag` to `CTF_USE_COMPLEX_AS_SIMPLE` on the mesh asset's BodySetup. This uses actual mesh geometry for collision traces.
- **Script:** `Scripts/fix_all_generators2.py` — scans ALL static meshes in the level, finds any with convex hull collision larger than 300 UU, and switches to complex-as-simple. Run this on any new level that uses purchased environment assets.
- **When to run:** After placing a new purchased environment level, or if player/creature gets stuck on invisible walls near large structures.
- Removing collision to "fix" pathing is a DEAD END — creatures need BlockAll surfaces to climb on.

---

## MAP GENERATION PIPELINE (in-game floor-plan minimaps)

RE/Silent-Hill style: one floor-plan texture per area, displayed by the Map widget (M / IA_Map) once
the player picks up that area's map item. C++ foundation done (session 39): `AZP_MapVolume`
(AreaID / AreaDisplayName / MapTexture / CaptureHeight / AreaBounds), `UZP_MapComponent`,
`UZP_MapWidget`, `AZP_MapPickup`, `ZP_MapTypes`.

**The generator:** `Scripts/Python/generate_floor_plan.py` — grid of downward line traces over a
MapVolume's bounds → RE-style plan (wall outlines on dark) → imports `T_Map_<AREA_ID>` to
`/Game/TheSignal/Textures/Maps/` → assigns it to the volume → saves the level. (Alt:
`capture_map.py` = top-down orthographic photo capture using CaptureHeight.)

**Per-floor pipeline:**
1. Open the floor `.umap`. New purchased-asset level → run `Scripts/fix_all_generators2.py` first
   (or interior walls won't register in the trace).
2. Place a `ZP_MapVolume` (edit + run `place_map_volume.py`, or by hand). Its **center Z must sit at
   room/player height** (the floor-plan scan starts at center Z and traces DOWN). Size AreaBounds to
   cover the floor footprint.
3. Set `AREA_ID` at the top of `generate_floor_plan.py` to the volume's AreaID.
4. Run it via the MCP Python endpoint (`POST :9847/api/python`).
5. Place a `ZP_MapPickup` for that area so the player finds the map in-world.

**Locked rule:** `AreaID` MUST match across MapVolume.AreaID == script AREA_ID == MapPickup.AreaID —
mismatched IDs = no map shows. Floor-plan scan height is the volume's CENTER Z, not CaptureHeight.

---

## CHECKPOINT PROTOCOL

`/checkpoints/` is this project's **troubleshooting knowledge base** — a Confluence/KB-style archive
of every working session (265+ entries and growing). Each file is a dated, self-contained record of
what changed, why, what broke, and what's still open. Treat it as the **institutional memory of the
project**, not as disposable scratch notes. When you write one, you are filing a KB article a future
session will rely on; when you hit a problem, the answer is often already filed.

### READ FIRST — checkpoints are a primary historical source
- **Before troubleshooting any bug, re-attempting a fix, or touching an unfamiliar system, SEARCH the
  checkpoints.** A past session very likely already hit it. E.g. `grep -ril "flashlight" checkpoints/`
  or `grep -rin "exposure\|lumen\|swimming pool" checkpoints/`. Use the Grep tool over `checkpoints/`.
- Checkpoints pair with the **DEAD ENDS** table below: DEAD ENDS = the one-line "never do this again"
  index; checkpoints = the full story behind each entry. If a checkpoint documents a confirmed failure,
  also add a DEAD ENDS row so it's indexed.
- After context **compaction**, the hook auto-injects only the **single most recent** checkpoint (by
  file mtime). That is NOT the whole history — the rest of the folder is still there. `grep` it; never
  assume the one re-injected file is all that exists.

### WHEN to write one (enforced by `.claude/hooks/checkpoint_monitor.py`)
- The counter resets to 0 whenever a new `.md` lands in `/checkpoints/`. The **Stop** hook BLOCKS
  stopping once **60 tool calls** (TOOL_CALL_THRESHOLD) have passed since the last checkpoint — so
  write one *well before* then, not only when the hook nags.
- The **PreCompact** hook warns near the context limit: write a checkpoint IMMEDIATELY when you see it,
  before context is compacted and detail is lost.
- Also write one: at the end of a session, after solving a non-obvious problem, and before any risky,
  destructive, or hard-to-reverse operation.
- GOTCHA: the counter only resets when a **new `.md` filename** lands in `/checkpoints/`. Overwriting
  an existing checkpoint does NOT reset it — write a new dated file (several per day is normal here).

### NAMING
`YYYY-MM-DD_short_snake_topic.md` (e.g. `2026-06-22_editor_pie_exposure_gamesettings.md`). Use a
**descriptive, keyword-rich** topic so future greps hit it. The newest mtime is the file the
SessionStart hook restores after compaction — keep the most recent checkpoint current and complete.

### FORMAT (match the existing files)
```
# Checkpoint - YYYY-MM-DD - <one-line headline of the outcome>

## State
What is true right now: the system worked on, current status, what's verified vs unverified.

## What changed / how it works
The actual change, the mechanism, key values, the reasoning — enough for a future session to
reconstruct it without re-deriving from scratch.

## STILL OPEN / NEXT
Open tickets, the next concrete step, anything BLOCKED on the dev.

## GOTCHAS
Traps, surprises, "looks done but isn't," engine/tool quirks discovered this session.

## Files / artifacts
Source/asset/script paths touched, memory entries added, related checkpoints/tickets by name.
```
Keep it factual and plain — a future Claude (or the dev) must be able to act on it cold. Cross-link
related checkpoints by filename and DEAD ENDS rows by date.

---

## DEAD ENDS

Document every failed approach here so no session re-attempts them.

| Date | Approach | Why It Failed |
|------|----------|---------------|
| 2026-03-01 | Removing fence/tank collision (OverlapAllDynamic) to fix creature pathing | Removes climbing surfaces — creatures need BlockAll to climb |
| 2026-03-01 | Reducing BP_3D_Grid Z to 400 to speed bake | Creature can't path over walls — Z must cover wall tops (1000+) |
| 2026-02-28 | `HideBoneByName("pelvis")` on PlayerMesh | Hides entire body recursively — no per-bone override control |
| 2026-02-28 | `SK_Shirt_01a_FPP` + `ABP_TacticalShooter_UE5` | Invisible — confirmed dead end x2 |
| 2026-02-28 | `SetLeaderPoseComponent` across camera/capsule attachment parents | Broken bone orientation — different transform hierarchies |
| 2026-02-28 | Shadow-only inherited Mesh without GASP AnimBP | T-pose or broken orientation — needs GASP dual-mesh architecture |
| 2026-02-28 | Camera-only CrouchCameraOffsetZ for smooth crouch | Arms/weapon snap instantly while camera lerps — must offset entire PlayerMesh Z |
| 2026-03-02 | Crest phase (horizontal push over fence top) for creature movement | Code ordering bug (wall normal cleared before ceiling check). Even after fix, multi-frame slide looked buggy. Replaced with ballistic launch toward player. |
| 2026-03-02 | Stateless CMC (zero state variables) for creature climbing | CalcVelocity always outputs toward waypoint, SafeMove blocked at Time=0, need frame-to-frame wall state for pre-move velocity redirect |
| 2026-03-02 | CalcVelocity + custom CMC climb redirect coexistence | Plugin CalcVelocity fights CMC redirect every frame. Every climbing fix breaks ground movement, every ground fix breaks climbing. SOLVED by removing CalcVelocity entirely — CMC owns all velocity. |
| 2026-03-03 | MCP material pin connections (connect_material_pins) | Connections report success but don't persist through editor save/reload. Editor's in-memory state overwrites MCP's disk changes. SOLVED by using UE5 Python API (unreal.MaterialEditingLibrary) via MCP Python endpoint instead. |
| 2026-03-03 | MCP set_expression_value for VectorParameter | XML params always serialize as strings, VectorParameter needs object {r,g,b,a}. Cannot set name or value of VectorParameters via MCP. Use Python API instead. |
| 2026-03-03 | Parameters.TexCoords[0] in UI domain Custom HLSL | UI domain materials don't populate FMaterialPixelParameters.TexCoords. Must use TextureCoordinate node wired as Custom input pin instead. |
| 2026-03-09 | `indirect_lighting_intensity = 0` to fix Lumen swimming pool effect | No visible difference. The swimming pool is Lumen GI probes, not indirect intensity. Must disable GI method entirely (`dynamic_global_illumination_method = NONE`). |
| 2026-03-09 | Deleting emissive light fixture meshes (SM_LampBig, SM_Florosent) to fix swimming pool | Wrong root cause. The swimming pool effect is fundamental to Lumen GI, not caused by specific meshes. Incorrectly deleted fixtures. |
| 2026-03-09 | `auto_exposure_bias` on PPV to control editor viewport brightness | No visible effect in editor viewport. Use `indirect_lighting_intensity` instead (0.0025 = good horror level). |
| 2026-03-09 | DefaultEngine.ini / console commands to disable Lumen GI | Editor caches settings, ignores ini changes. Console commands target wrong context. Only PPV `dynamic_global_illumination_method = NONE` works. |
| 2026-06-12 | Lowering PlayerMesh relative Z to drop arms off screen (weapon swap dip) | The FP camera is socketed to PlayerMesh (FPCamera) — moving the mesh moves the VIEW; player sees a forced crouch. NEVER translate PlayerMesh for transitions. Arm motion must come from animation (ToggleReadyPose, montages) or bone hiding. |
| 2026-06-24 | Persisting enemy death via `Destroy()` + EGUI `RegisterDestruction` | Permanent and one-way — EGUI has NO un-register API and never recreates a destroyed actor, so objective-driven revival is impossible. Persist STATE instead: keep the corpse actor, save `bIsDead`, restore via IZP_Revivable on load, revive by resetting to alive. RegisterDestruction MUST be false on enemy EGUI comps. See checkpoint 2026-06-24_enemy_death_persistence_and_objective_revival. |
| 2026-06-24 | Reparenting a pack BP (root = `DefaultSceneRoot`) to a C++ class that has its OWN native root component (e.g. `BP_Elevator_1` → `AZP_Elevator`) to "make it the right class" | Reparent compiles/saves "successfully" but the BP's SCS mesh subtree under DefaultSceneRoot DOES NOT INSTANTIATE — spawned instance has only the inherited native components; `list_components` still lists the ghosts so it looks fine. The native root wins and the old SCS root's children are dropped. FIX: build a fresh BP child of the C++ class and ADD the meshes as components (set PlatformMesh + child SMCs via SubobjectDataSubsystem), copying mesh+relative-transform from the pack BP. Always spawn-verify a reparented BP. See checkpoint 2026-06-24_inner_map_elevator_transit_system. |
| 2026-06-29 | Treating the Shambler `EXCEPTION_ACCESS_VIOLATION` on anim load (PIE/-game/level open) as a load-path bug — deferring `LoadAnimDefaults` to next tick, switching to `RequestAsyncLoad`, or re-saving the asset in a commandlet | None fixed it; all crash identically (`UnrealEd!… → CoreUObject load`). It's NOT sync-vs-async, timing, or position-8 — it's the ASSET. The retargeted `A_Shambler_Hit_Front/Back` carried orphan GASP curves (`DistanceToApex`/`DistanceCurve`/`blendOrient1`) the necromorph skeleton can't name-map, so `UAnimSequence::PostLoad`'s editor curve check null-derefs. Re-save round-trips the bad curves untouched. FIX = strip the orphan curves (`AnimationLibrary.remove_curve` per RCT_FLOAT name). See checkpoint 2026-06-29_shambler_anim_curve_postload_crash. |
| 2026-06-29 | "Fixing" that same crash by hard-referencing the Shambler anim clips on `BP_Shambler` so they load with the level | Made it WORSE — moves the load into the level package, so the editor crashes on OPENING ResearchFacility (the EditorStartupMap), not just on play. Reverted. The real fix is cleaning the asset (strip curves), not changing where/when it loads. |

---

## DEVELOPER CONTEXT

- **Solo developer project.** Architecture optimizes for: one person navigating the codebase months later, fast iteration over theoretical perfection, clear ownership of every system.
- **Strengths:** Automated UE5 workflows (animations, node construction), professional audio production (Ableton Live 10, multi-instrument, $3k+ recording studio), narrative writing.
- **Weaknesses:** Enemy AI systems, complex gameplay programming beyond environment/automation.
- **Bias toward simplicity.** A simple system that works > an elegant system that takes weeks to debug alone.
- **Campaign build phase.** Core systems proven via vertical slice. Now building the actual game — levels, NPCs, narrative, pacing.

---

## CODING PHILOSOPHIES

1. **Interface-first architecture.** All gameplay communication through Blueprint Interfaces. No direct BP-to-BP coupling.
2. **Event-driven, not poll-driven.** Dispatchers and event bindings. No Tick polling without documented justification.
3. **Data-driven tuning.** No magic numbers. DataAssets and DataTables for all tunable values.
4. **Subsystem ownership.** Every BP actor has a named owner subsystem in a comment block.
5. **Naming is architecture.** Conventions locked. Enforced by Python validation. No renaming after creation.
6. **Automate the repetitive, author the creative.** Node patterns = automated. Narrative pacing, lighting composition, audio layering = handcrafted.
7. **Fail loud.** Validation checks surface errors visibly. On-screen debug messaging in dev. ensure/check macros in C++. Log aggressively.
8. **Production quality.** Past prototype phase — build it right the first time. Systems are proven, now building the actual game.
