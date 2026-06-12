"""
Night backdrop for isolated-floor demo maps (QuakeCon) — Building1_3rdFloor.

1. Re-centers + scales the NightGround grass plane so it reads "indefinite".
2. Rings the building with silhouette hills (scaled SM_Rock) and treeline
   clumps (scaled SM_Bush). At 2 AM these read as dark forest/hill shapes.

Re-runnable: deletes existing Backdrop_* actors first, re-rolls from SEED.
Tunables below. Spawned actors: folder NightBackdrop, collision OFF.
"""
import unreal, random, math

SEED = 1203
CENTER = unreal.Vector(-11105.0, -389.0, 0.0)   # building footprint center
GROUND_Z = -15.0
GRASS_SCALE = 2500.0          # 100UU plane -> 2.5 km square
HILL_MESH = '/Game/office_StarterContent/Props/SM_Rock'
TREE_MESH = '/Game/office_StarterContent/Props/SM_Bush'
HILLS = 14                    # ring count
HILL_R = (150000.0, 200000.0) # 1.5–2 km
HILL_H = (8000.0, 15000.0)    # 80–150 m tall
HILL_BURY = 0.30              # fraction sunk into ground
TREE_CLUMPS = 10
TREES_PER_CLUMP = (4, 8)
TREE_R = (120000.0, 170000.0) # 1.2–1.7 km
TREE_H = (1500.0, 2500.0)     # 15–25 m tall
CLUMP_SPREAD = 6000.0
# concrete apron around the building footprint
SLAB_MAT = '/Game/MilitaryAirport/Materials/Tileable/MI_ConcreteGround'
SLAB_X = (-24256.0, 2045.0)   # building footprint
SLAB_Y = (-2155.0, 1378.0)
SLAB_MARGIN = 2000.0          # 20 m apron beyond walls
SLAB_THICK = 30.0             # cm
SLAB_TOP_Z = -12.0            # just above the grass plane (-15)

rng = random.Random(SEED)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert '3rdFloor' in w.get_name(), f'wrong map: {w.get_name()}'

# --- clean previous run ---
removed = 0
for a in list(actor_sub.get_all_level_actors()):
    if a.get_actor_label().startswith('Backdrop_'):
        actor_sub.destroy_actor(a)
        removed += 1

# --- grass: re-center under building, scale up ---
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == 'NightGround':
        a.modify()
        a.set_actor_location(unreal.Vector(CENTER.x, CENTER.y, GROUND_Z), False, False)
        a.set_actor_scale3d(unreal.Vector(GRASS_SCALE, GRASS_SCALE, 1.0))
        unreal.log(f'grass: centered ({CENTER.x:.0f},{CENTER.y:.0f}), {GRASS_SCALE * 100 / 100000:.1f} km square')
        break

hill_mesh = unreal.load_asset(HILL_MESH)
tree_mesh = unreal.load_asset(TREE_MESH)
assert hill_mesh and tree_mesh, 'backdrop meshes missing'

def spawn(mesh, label, x, y, height, bury_frac):
    b = mesh.get_bounding_box()
    mh = b.max.z - b.min.z
    s = height / mh
    sxy = s * rng.uniform(1.4, 2.4)  # wider than tall = rolling silhouette
    a = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, 0))
    c = a.get_components_by_class(unreal.StaticMeshComponent)[0]
    c.set_editor_property('static_mesh', mesh)
    a.set_actor_scale3d(unreal.Vector(sxy, sxy, s))
    z = GROUND_Z - b.min.z * s - height * bury_frac
    a.set_actor_location(unreal.Vector(x, y, z), False, False)
    a.set_actor_rotation(unreal.Rotator(roll=0, pitch=0, yaw=rng.uniform(0, 360)), False)
    a.set_actor_label(label)
    a.set_folder_path('NightBackdrop')
    a.set_actor_enable_collision(False)
    return a

n_hills = 0
for i in range(HILLS):
    ang = (i / HILLS) * 2 * math.pi + rng.uniform(-0.15, 0.15)
    r = rng.uniform(*HILL_R)
    spawn(hill_mesh, f'Backdrop_Hill_{i:02d}',
          CENTER.x + r * math.cos(ang), CENTER.y + r * math.sin(ang),
          rng.uniform(*HILL_H), HILL_BURY)
    n_hills += 1

n_trees = 0
for i in range(TREE_CLUMPS):
    ang = (i / TREE_CLUMPS) * 2 * math.pi + rng.uniform(-0.2, 0.2)
    r = rng.uniform(*TREE_R)
    cx, cy = CENTER.x + r * math.cos(ang), CENTER.y + r * math.sin(ang)
    for j in range(rng.randint(*TREES_PER_CLUMP)):
        spawn(tree_mesh, f'Backdrop_Tree_{i:02d}_{j}',
              cx + rng.uniform(-CLUMP_SPREAD, CLUMP_SPREAD),
              cy + rng.uniform(-CLUMP_SPREAD, CLUMP_SPREAD),
              rng.uniform(*TREE_H), 0.05)
        n_trees += 1

# --- concrete apron slab ---
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
slab_mat = unreal.load_asset(SLAB_MAT)
assert cube and slab_mat, 'slab mesh/material missing'
sx = (SLAB_X[1] - SLAB_X[0] + 2 * SLAB_MARGIN) / 100.0
sy = (SLAB_Y[1] - SLAB_Y[0] + 2 * SLAB_MARGIN) / 100.0
sz = SLAB_THICK / 100.0
slab = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(
    (SLAB_X[0] + SLAB_X[1]) / 2.0, (SLAB_Y[0] + SLAB_Y[1]) / 2.0,
    SLAB_TOP_Z - SLAB_THICK / 2.0))
sc = slab.get_components_by_class(unreal.StaticMeshComponent)[0]
sc.set_editor_property('static_mesh', cube)
sc.set_material(0, slab_mat)
slab.set_actor_scale3d(unreal.Vector(sx, sy, sz))
slab.set_actor_label('Backdrop_ConcreteSlab')
slab.set_folder_path('NightBackdrop')
slab.set_actor_enable_collision(False)
unreal.log(f'[backdrop] slab: {sx:.0f}m x {sy:.0f}m, top @Z{SLAB_TOP_Z}')

unreal.log(f'[backdrop] removed {removed} old, spawned {n_hills} hills + {n_trees} trees')
ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'[backdrop] level saved: {ok}')
