import unreal, random
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
random.seed(7)

GROUND_TOP = 485.0

# re-runnable: clear prior ground/forest
cleared = 0
for a in eas.get_all_level_actors():
    l = a.get_actor_label()
    if l.startswith("Ground_") or l.startswith("Forest_Tree_"):
        eas.destroy_actor(a); cleared += 1
if cleared: print("cleared prior env actors:", cleared)

CUBE  = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_Cube.SM_Cube")
GRASS = unreal.load_asset("/Game/office_StarterContent/Materials/M_Ground_Grass")

def setmat(actor, mat):
    c = actor.get_components_by_class(unreal.StaticMeshComponent)[0]
    for i in range(c.get_num_materials()):
        c.set_material(i, mat)

# ---- tiled concrete MI (M_Mat 'Tilling' vector, like the carpet) ----
conc_parent = unreal.load_asset("/Game/Office/OfficeMats/MI_Concrete01.MI_Concrete01")
cfull = "/Game/Office/OfficeMats/MI_Concrete01_Ground"
if EAL.does_asset_exist(cfull):
    concMI = unreal.load_asset(cfull)
else:
    concMI = tools.create_asset("MI_Concrete01_Ground","/Game/Office/OfficeMats",
              unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    concMI.set_editor_property("parent", conc_parent)

# ---- try to tile the grass too (give it its own MI if a tiling param exists) ----
grass_to_use = GRASS
try:
    vnames = list(MEL.get_vector_parameter_names(GRASS))
    snames = list(MEL.get_scalar_parameter_names(GRASS))
    print("grass vec params:", vnames, "scalar:", snames)
except Exception as e:
    vnames, snames = [], []
    print("grass param read failed:", e)

# ---- footprint / apron ----
AX_C, AY_C = 3755.0, 2423.0
AX_FULL, AY_FULL = 21504.0, 14406.0
MEL.set_material_instance_vector_parameter_value(concMI, "Tilling",
        unreal.LinearColor(AX_FULL/200.0, AY_FULL/200.0, 0.0, 0.0))   # ~2m concrete squares
EAL.save_asset(cfull)

# GRASS ground (big, under everything)
grass = eas.spawn_actor_from_object(CUBE, unreal.Vector(-3750.0, 2000.0, GROUND_TOP-10.0))
grass.set_actor_scale3d(unreal.Vector(700.0, 700.0, 0.2))
setmat(grass, grass_to_use)
grass.set_actor_label("Ground_Grass")

# CONCRETE apron around the building
conc = eas.spawn_actor_from_object(CUBE, unreal.Vector(AX_C, AY_C, 478.0))
conc.set_actor_scale3d(unreal.Vector(AX_FULL/100.0, AY_FULL/100.0, 0.18))
setmat(conc, concMI)
conc.set_actor_label("Ground_ConcreteApron")

# ---- FOREST: dense tall spruce, starting just past the backdrop buildings ----
trees = [unreal.load_asset("/Game/PN_interactiveSpruceForest/Meshes/full/high/spruce_full_02"),
         unreal.load_asset("/Game/PN_interactiveSpruceForest/Meshes/full/high/spruce_full_03"),
         unreal.load_asset("/Game/PN_interactiveSpruceForest/Meshes/half/low/spruce_half_03_low")]
base_local = [-26.0, -2.0, -30.0]   # mesh base offset from pivot

INNER = (-19000.0, 11500.0, -12000.0, 16000.0)   # backdrop-building ring (+margin): NO trees inside
OUTER = (-33000.0, 25500.0, -26000.0, 30000.0)
step = 2200.0
n = 0
y = OUTER[2]
while y <= OUTER[3]:
    x = OUTER[0]
    while x <= OUTER[1]:
        inside_ring = (INNER[0] <= x <= INNER[1]) and (INNER[2] <= y <= INNER[3])
        if not inside_ring:
            i = random.randint(0, 2)
            s = random.uniform(2.6, 4.2)
            yaw = random.uniform(0, 360)
            z = GROUND_TOP - base_local[i]*s
            act = eas.spawn_actor_from_object(trees[i],
                    unreal.Vector(x+random.uniform(-550,550), y+random.uniform(-550,550), z),
                    unreal.Rotator(0,0,yaw))
            if act:
                act.set_actor_scale3d(unreal.Vector(s, s, s))
                n += 1
                act.set_actor_label("Forest_Tree_%04d" % n)
        x += step
    y += step

print("grass + concrete apron placed; spruce trees placed:", n)
print("level saved:", les.save_current_level())
