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
- **Playtime:** TBD (scope determined after vertical slice — NOT locked)
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
| `PlayerCharacter` | planned | Grace's first-person controller, movement, interaction |
| `InteractionSystem` | planned | Object interaction, pickup, examine, use |
| `NarrativeTriggerSystem` | planned | Story beat triggers, found narrative activation, progression gates |
| `RenCommunicationSystem` | planned | Ren's voice delivery, earpiece simulation, dialogue management, environmental audio manipulation |
| `InventorySystem` | planned | Weapons, key items, found documents |
| `LightingMoodSystem` | planned | Dynamic lighting states, Ren's influence on facility lighting, mood transitions per zone |
| `AudioStateSystem` | planned | MetaSounds ambient layers, Ren's audio signature, radio-as-instrument processing, spatial audio |
| `FacilitySystemsManager` | planned | Doors, power routing, security terminals, electronics Ren can manipulate |
| `SaveSystem` | planned | Checkpoint-only saves (v0.1), player transform + inventory + narrative flags + facility states |
| `DebugOverlay` | planned | On-screen system status, validation errors, prototype telemetry |

---

## PROTOTYPE SCOPE (v0.1)

**One linear level: Research Campus Corridor**
- Power flickering, one locked door puzzle, first Ren communication via earpiece
- **Win condition:** Grace reaches the server room
- **No combat** — tension through sound, lighting, and environmental storytelling only

### Must Demonstrate:
- [ ] First-person movement that feels grounded and vulnerable (not military FPS)
- [ ] Ren's voice delivery through earpiece (audio processing, spatial placement)
- [ ] One environmental puzzle gated by facility systems (power routing or security clearance)
- [ ] Lighting as narrative tool (Ren guiding through light, darkness as absence of Ren)
- [ ] At least one found narrative element (document, email, recording)
- [ ] The emotional hook: Grace is scared, Ren is calm, player starts to trust the voice

---

## GDD SYNC RULE

- Any built mechanic must be documented in `/Docs/GDD_Current.md` before the next system begins
- Removed mechanics go to `/Docs/GDD_Deprecated.md` with reason and date
- Canonical GDD lives externally as .docx (v0.2.1). `/Docs/GDD_Current.md` is the in-repo working mirror

---

## SAFEGUARDS

- **Never delete assets** — deprecate with `DEP_` prefix, move to `_DEPRECATED/` folder
- **No work directly on main** — all prototype work on branch: `proto/corridor-v1`
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
| Create VariableGet for OTHER Blueprint's variables | Only local variables via MCP |
| `connect_pins` batch mode | Individual calls only |
| `duplicate_nodes` preserving connections | Must rewire manually after |
| DynamicCast without exec pin connection | Gets pruned by compiler |

**Critical:** Saving a BP via MCP can trigger Blueprint Reinstancing on referenced BPs, resetting component property overrides in editor memory. Restart editor if properties revert.

---

## UE5 EXEC PIN RULE (NightShadow Lesson)

Exec input pins accept **only ONE connection.** Connecting a new wire **silently overwrites** the previous connection. No warning.

**Before wiring any exec pin via MCP:** Always check existing connections with `get_pin_info` first. If one exists, insert a Sequence node.

---

## DEAD ENDS

Document every failed approach here so no session re-attempts them.

| Date | Approach | Why It Failed |
|------|----------|---------------|
| — | (none yet) | — |

---

## DEVELOPER CONTEXT

- **Solo developer project.** Architecture optimizes for: one person navigating the codebase months later, fast iteration over theoretical perfection, clear ownership of every system.
- **Strengths:** Automated UE5 workflows (animations, node construction), professional audio production (Ableton Live 10, multi-instrument, $3k+ recording studio), narrative writing.
- **Weaknesses:** Enemy AI systems, complex gameplay programming beyond environment/automation.
- **Bias toward simplicity.** A simple system that works > an elegant system that takes weeks to debug alone.
- **Prototype permission is real.** Don't overengineer v0.1. Get the corridor working, get Ren talking, get the lights flickering. Then refactor.

---

## CODING PHILOSOPHIES

1. **Interface-first architecture.** All gameplay communication through Blueprint Interfaces. No direct BP-to-BP coupling.
2. **Event-driven, not poll-driven.** Dispatchers and event bindings. No Tick polling without documented justification.
3. **Data-driven tuning.** No magic numbers. DataAssets and DataTables for all tunable values.
4. **Subsystem ownership.** Every BP actor has a named owner subsystem in a comment block.
5. **Naming is architecture.** Conventions locked. Enforced by Python validation. No renaming after creation.
6. **Automate the repetitive, author the creative.** Node patterns = automated. Narrative pacing, lighting composition, audio layering = handcrafted.
7. **Fail loud.** Validation checks surface errors visibly. On-screen debug messaging in dev. ensure/check macros in C++. Log aggressively.
8. **Prototype permission.** "Good enough to test the idea" in prototype. Clearly marked. Never shipped as-is. Rebuilt clean when graduating to production.
