"""Create the reusable self-contained BP_InteractDoor.

Parented to AZP_InteractDoor. DoorMesh defaults to SM_DoorExit (a standalone
hinged door panel). OpenMode=Rotate, OpenAngle=90, InterpSpeed=4. Drag it into
any level and it works on its own — no separate trigger, no relinking.
Swap the DoorMesh's Static Mesh per-instance for other door types.
"""
import unreal

DEST_PATH = '/Game/Core/Actors'
BP_NAME = 'BP_InteractDoor'

# Pull the door mesh straight from the known exit door actor.
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
door_sm = None
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'F3_Exit_door_pCube166':
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            door_sm = c.get_editor_property('static_mesh')
        break
if not door_sm:
    raise RuntimeError("Could not resolve SM_DoorExit from F3_Exit_door_pCube166")
print(f"Default mesh resolved: {door_sm.get_name()} ({door_sm.get_path_name()})")

full = f"{DEST_PATH}/{BP_NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    raise RuntimeError(f"{full} already exists — aborting to avoid overwrite")

factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.ZP_InteractDoor)
atools = unreal.AssetToolsHelpers.get_asset_tools()
bp = atools.create_asset(BP_NAME, DEST_PATH, None, factory)
if not bp:
    raise RuntimeError("create_asset returned None")

# Set defaults on the CDO. DoorMesh is a C++ DefaultSubobject, so it's
# reachable on the CDO (unlike Blueprint SCS components).
gen_class = bp.generated_class()
cdo = unreal.get_default_object(gen_class)
mesh_comp = cdo.get_editor_property('DoorMesh')
mesh_comp.set_editor_property('static_mesh', door_sm)
cdo.set_editor_property('OpenMode', unreal.ZP_InteractDoorMode.ROTATE)
cdo.set_editor_property('OpenAngle', 90.0)
cdo.set_editor_property('InterpSpeed', 4.0)

unreal.EditorAssetLibrary.save_asset(full)

# Verify
cdo2 = unreal.get_default_object(unreal.load_asset(full).generated_class())
mc2 = cdo2.get_editor_property('DoorMesh')
sm2 = mc2.get_editor_property('static_mesh')
print(f"Created {full}: DoorMesh={sm2.get_name() if sm2 else 'NONE'}, "
      f"mode={cdo2.get_editor_property('OpenMode')}, angle={cdo2.get_editor_property('OpenAngle')}")
