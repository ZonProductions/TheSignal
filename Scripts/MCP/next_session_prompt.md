# Continue The Signal — Prototype Kickoff

Read `session_prompt.txt` and `CLAUDE.md` first. Everything from the kickoff doc has been set up: project structure, git, GCS cloud sync, BlueprintMCP, Python plugin, Content/ folder hierarchy, naming conventions, system registry.

## This Session: Start Building

### 1. Create proto/corridor-v1 branch
Branch from main. All work goes here per safeguards.

### 2. Base C++ Classes
Create the foundational C++ classes that Blueprint will extend:
- **TheSignalCharacter** — First-person character. Camera component, movement component tuned for vulnerability (not military FPS). Blueprint-exposed parameters for walk speed, sprint speed, head bob, camera sway. Owner: PlayerCharacter system.
- **TheSignalGameMode** — Sets default pawn, player controller, game state. Owner: Core.
- **TheSignalGameInstance** — Persistent state across levels (will hold save data later). Owner: Core.

These go in `Source/TheSignal/`. Follow the C++ header comment block requirements from CLAUDE.md.

### 3. Blueprint Extensions
After C++ compiles, create Blueprint extensions in the Content/ folders:
- `Content/Core/GM_TheSignal` (extends TheSignalGameMode)
- `Content/Core/GI_TheSignal` (extends TheSignalGameInstance)
- `Content/Core/PC_Grace` (Player Controller)
- `Content/Player/Blueprints/BP_Grace` (extends TheSignalCharacter)

### 4. Input Setup
Create Enhanced Input mapping context and actions in `Content/Player/Input/`:
- Move, Look, Sprint, Interact
- Grace-appropriate: no crouch, no jump (or very limited), no weapon swap

### 5. MAP_ProtoCorridor Blockout
Create `Content/Maps/MAP_ProtoCorridor`. Basic BSP/geometry blockout:
- Linear path from event hall entrance to server room
- One branch/dead-end for tension
- Placeholder door that will become the locked puzzle
- Lighting: overhead fluorescents that can be controlled per-zone

### 6. Verify BlueprintMCP
With the editor open, call `server_status` to confirm the MCP bridge is live. Then test Python execution inside the editor.

**Remember: Prototype permission is real. Get it working, not perfect.**
