# The Signal — CLAUDE.md

## ABSOLUTE RULES — NEVER VIOLATE

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
- **Protagonist:** Grace Owens — failed actor/thespian, male, late 20s–early 30s. Takes any gig to survive. Recently fired from a cancelled TV show. Working a corporate event at a high-tech research campus when containment breach occurs.
- **Antagonist/Guide:** Ren — a primordial entity that communicates through radio frequencies and electronics. Presents as a warm, calm, helpful human voice through Grace's earpiece. Chose the name "Ren" because a name makes something feel human. True goal: inhabit autonomous exoskeletons from a cancelled defense project. Every act of help moves Grace closer to reactivating that hardware.
- **Core Premise:** Grace thinks he's escaping. He's assembling the key.
- **"The Signal"** is the meta-level name for what Ren is. To Grace, it is only ever **Ren.**

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
- **BioShock (Atlas)** — A guide whose helpfulness IS the deception
- **BioShock Infinite** — World-building through environment, companion relationships
- **The Last of Us 1/2** — Emotional storytelling benchmark
- **Futurama God Entity** — Ren's true nature. Vast. Doesn't need to perform power.

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
- **Grace is not a soldier.** Animation must communicate vulnerability — unfamiliar weapon sway, panicked sprinting, anxious idle states.
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
| `BPC_` | Blueprint Components | `BPC_RenAudioSource` |
| `BPI_` | Blueprint Interfaces | `BPI_FacilityDevice` |
| `ABP_` | Animation Blueprints | `ABP_Grace` |
| `DA_` | Data Assets | `DA_RenDialogue` |
| `DT_` | Data Tables | `DT_InventoryItems` |
| `WBP_` | Widget Blueprints | `WBP_InteractionPrompt` |
| `SS_` | Subsystems | `SS_FacilitySystems` |
| `E_` | Enumerations | `E_DoorState` |
| `S_` | Structures | `S_InventoryItem` |
| `M_` | Materials | `M_ConcreteFloor` |
| `MI_` | Material Instances | `MI_ConcreteFloor_Dirty` |
| `T_` | Textures | `T_ConcreteFloor_BC` |
| `SM_` | Static Meshes | `SM_LabDesk` |
| `SK_` | Skeletal Meshes | `SK_Grace` |
| `SFX_` | Sound Effects | `SFX_DoorSlam` |
| `MS_` | MetaSound Sources | `MS_RenVoiceProcessor` |
| `SQ_` | Level Sequences | `SQ_IntroEvent` |
| `MAP_` | Maps/Levels | `MAP_ProtoCorridor` |
| `GI_` | Game Instance | `GI_TheSignal` |
| `GM_` | Game Mode | `GM_TheSignal` |
| `PC_` | Player Controller | `PC_Grace` |
| `DEP_` | Deprecated Assets | Moved to `_DEPRECATED/` folder |

---

## SYSTEM REGISTRY

| System | Status | Description |
|--------|--------|-------------|
| `PlayerCharacter` | done | Grace's first-person controller, movement, interaction, Kinemation integration |
| `InteractionSystem` | done | Object interaction, pickup, examine, use (BPI_Interactable) |
| `NarrativeTriggerSystem` | planned | Story beat triggers, found narrative activation, progression gates |
| `RenCommunicationSystem` | planned | Ren's voice delivery, earpiece simulation, dialogue management, environmental audio manipulation |
| `InventorySystem` | done | Weapons, key items, found documents (Moonville + C++ bridge) |
| `LightingMoodSystem` | planned | Dynamic lighting states, Ren's influence on facility lighting, mood transitions per zone |
| `AudioStateSystem` | planned | MetaSounds ambient layers, Ren's audio signature, radio-as-instrument processing, spatial audio |
| `FacilitySystemsManager` | planned | Doors, power routing, security terminals, electronics Ren can manipulate |
| `SaveSystem` | done | Save/load with 5 slots, save point actors, screenshot capture |
| `DebugOverlay` | planned | On-screen system status, validation errors, telemetry |

---

## PROJECT PHASE — Campaign Build

**Vertical slice complete.** Core systems proven: player character, combat, creatures (Crawler), inventory, save/load, UI/HUD, interaction, lock/key puzzles. TreatmentStation served as the proving ground.

**Now building the actual campaign:**
- Level design across the full game structure
- NPC design and implementation
- Narrative implementation (Ren dialogue, story beats, found narrative)
- Additional creature types beyond Crawler
- Full game pacing and progression

### Character Creation
- **Character Customizer (mlindborg)** + **Modern Clothes Pack** — purchased for all human character creation (Grace, NPCs, extras, corpses)
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
