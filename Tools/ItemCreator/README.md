# The Signal — Item Creator

One tool to mint Moonville `PDA_Item` DataAssets for The Signal, in two flavors:

- **Objective Item** — fuse / record / battery / etc. Referenced by `BP_ObjectiveContainer.RequiredItems`
  and placed in the world via `BP_ItemPickup`.
- **Note / Document** — collectible logs (supersedes the old `note_creator.py`).

It creates `DA_<Name>` under `/Game/Core/Items/` with a chosen **static mesh**, **display name**, optional
**icon**, stack size, and the right **gameplay tag** (`Item.Objective` / `Item.Note`).

## Run
1. Open the UE5 editor (BlueprintMCP must be active — endpoint `http://localhost:9847`).
2. Double-click **`dist/ItemCreator.exe`** (or `python item_creator.py`).
3. Pick a Type, enter a name (Asset ID auto-fills), paste the **Static Mesh** path
   (right-click the mesh in the Content Browser → *Copy Reference*, strip the `StaticMesh'...'` wrapper —
   the `/Game/...` part), optionally an icon, then **Create Item**.

## How it fits the objective-container system
The item's mesh + name live on the **DA** (single source of truth). So:
1. Create `DA_Fuse` here (mesh = your fuse mesh).
2. In `BP_ObjectiveContainer` → `RequiredItems`, add `{ DA_Fuse, 3 }`.
3. Place 3 `BP_ItemPickup` actors, each with `ItemDataAsset = DA_Fuse` (the pickup auto-shows the mesh).
4. Player collects 3 fuses → deposits → objective completes. Reuse the same pattern for record→player,
   batteries→remote, etc.

## Rebuild the exe
After editing `item_creator.py`: `powershell -ExecutionPolicy Bypass -File build_exe.ps1`
(requires `pip install pyinstaller`). Artifacts land in `build/` and `dist/` inside this folder.
