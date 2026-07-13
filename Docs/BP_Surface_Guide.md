# BP_Surface — Procedural Surface Actor (Guide)

**C++ class:** `ASM_Surface` (`Source/TheSignal/SM_Surface.h/.cpp`)
**Blueprint:** `/Game/Core/Actors/BP_Surface`
**Owner subsystem:** FacilitySystemsManager (level architecture helpers)
**Status:** good-enough-for-now, locked 2026-07-12 (dev). Placed in ResearchFacility as
`Ceiling2` (mesh-tile ceiling) and `BP_Surface2` (flat slab).

## What it is
A drop-in flat surface (floor / roof / ceiling) that:
- fills a designer-sized rectangle (`AZP_SurfaceSize`, centered on the actor, top face at actor Z),
- cuts REAL holes — visibility AND collision — around any actors you assign
  (`AZP_CutActors`: a stair, elevator shaft walls, ...),
- live-follows those actors in the editor: move/resize a cut actor and the surface rebuilds
  itself (every `AZP_FollowInterval` seconds, plus on any property edit or actor move).
  No scripts, no manual refresh.

## Two modes

### 1. Mesh-tile mode (set a Static Mesh on TileISM)
Select the **TileISM** component and set its ordinary **Static Mesh** field (e.g. `SM_Floor_3`).
The surface fills with real instances of that mesh.

**Guarantees (the 2026-07-12 contract):**
- **No stretching, ever.** Every tile renders at the mesh's natural footprint, scale (1,1,1).
  There is no scale math in the code path at all.
- **Resizing adjusts the grid, not the tiles.** Growing `AZP_SurfaceSize` adds rows/columns.
- **Swapping meshes never changes the shape.** The grid always covers the FULL
  `AZP_SurfaceSize` rectangle: tile count rounds UP and the grid is centered, so the partial
  row/column overhangs the edge by up to half a tile per side instead of scaling or shrinking
  the surface.
- **Cuts knock out whole tiles.** Any tile overlapping a cut actor's footprint (beyond 1uu
  tolerance) is removed — holes are tile-quantized. Tighten or widen with `AZP_CutMargin`.

Clear the Static Mesh to fall back to slab mode.

### 2. Slab-tile mode (TileISM mesh empty)
A grid of engine-cube tiles on the **SlabISM** component: `AZP_TileSize` pitch, `AZP_Thickness`
thick, top at the actor's Z. Set your material on SlabISM's Materials slot — cube UVs are 0-1
per face, so **every tile shows exactly ONE clean pattern repeat** (uniform grid, no stretching;
`AZP_TileSize` = the pattern size you want per tile). Same grid rules as mesh mode: full
rectangle coverage, whole-tile cut knockout.

**Why cubes and not a procedural mesh (2026-07-12):** the original ProceduralMeshComponent slab
had two fatal PIE bugs — PMC generates no runtime distance fields, so **Lumen rendered it pitch
black**, and CMC **landing failed on its trimesh** (post-dodge `LaunchCharacter` never re-landed
→ friction-free ice-slide until reaching a real floor). Cube instances are real static meshes:
Lumen lights them and the character lands on their box collision like any hand-placed floor.
`SurfaceMesh` (the PMC) survives as an inert legacy root only — no collision, no rendering.

## Materials — you own them, the actor never writes them
Set materials the normal UE way: select **TileISM** (mesh mode) or **SlabISM** (slab mode)
and override the slot under **Details ▸ Materials**. Nothing re-stamps them on rebuild.
- Mesh mode with no override = the mesh's own materials (the usual case for pack floor tiles).
- Slab mode with no override = the plain BasicShapeMaterial — assign one.

## Change log (2026-07-12, all three iterations dev-driven)
1. **Stripped the material override** that painted `MI_Concrete_5_TwoSided` over the tile mesh
   ("layering a material and surface on top of itself").
2. **Killed non-uniform tiling.** Original code inserted cut-actor edges as global grid lines —
   one hole split entire rows/columns (observed: 62 tiles, 62 distinct scales). An intermediate
   uniform-fit version still scaled tiles to fit and was rejected: any pattern scaling reads as
   stretching.
3. **Locked the no-stretch contract** (natural-size tiles, whole-tile holes, full-rectangle
   coverage with overhang) and **REMOVED three knobs**:
   - `AZP_Material` — its BP default re-stamped the ISM material slot on every rebuild,
     fighting manual material edits ("stale element in materials"). Materials now live only on
     the components.
   - `bAZP_AutoTileFromMesh`, `bAZP_ScaleEdgeTiles` — meaningless once tiles can't scale.
   Deleted properties drop harmlessly from saved actors on next load ("unknown property" log
   once, gone after resave). No CoreRedirects needed for deletions.

## Gotchas
- NEVER rebuild the slab as a ProceduralMeshComponent: PMC has no runtime distance fields →
  pitch black under Lumen, and CMC landing fails on its trimesh → post-dodge ice-slide
  (checkpoint 2026-07-12_sm_surface_pmc_retired_cube_slab).
- A small `AZP_TileSize` legitimately builds thousands of cubes (still one ISM = one draw call),
  but the pattern gets dense — raise `AZP_TileSize` for bigger tiles.
- The overhang means covered area can exceed `AZP_SurfaceSize` by up to half a tile per side —
  overhanging tiles (and their collision) can poke into/through adjacent walls. Usually hidden
  inside wall thickness; size the rectangle to the room interior.
- Whole-tile knockout means holes can be up to one tile larger than the cut actor's bounds.
  Negative `AZP_CutMargin` shrinks the tested rect if a hole eats one row too many.
- Mesh-tile pitch = the mesh's bounding-box footprint. A mesh with skirt/trim geometry larger
  than its visual tile will grid with gaps — use clean slab meshes (the pack SM_Floor_* are fine).
- Scripts must find placed surfaces by class (`SM_Surface` / `BP_Surface_C`), not by label —
  the dev renames instances (e.g. `Ceiling2`).
- `SM_Floor_3` exists in THREE pack folders (ResearchFacility / ResearchCenter / SecretBase);
  ResearchFacility's variant matches that level's hand-built floors.

## Related
- Checkpoint: `checkpoints/2026-07-12_sm_surface_no_stretch_tile_grid.md` (full session trail)
- Knob registry entry: `Docs/AZP_CustomKnobs.md` ▸ SM_Surface
