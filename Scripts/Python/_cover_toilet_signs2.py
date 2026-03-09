import unreal

# Create a simple deferred decal material and place decal actors over the toilet signs.
# Decals project onto geometry — perfect for covering a sign on a door surface.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# First, create a simple solid-color deferred decal material via Python
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

mat_path = '/Game/TheSignal/Materials/M_FlatCover'
mat = unreal.load_asset(mat_path)
if not mat:
    # Create a new material
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset('M_FlatCover', '/Game/TheSignal/Materials', unreal.Material, factory)
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('decal_blend_mode', unreal.DecalBlendMode.DBM_TRANSLUCENT)

    # Add a constant color node — dark grey to match door
    color_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, 0)
    color_node.set_editor_property('constant', unreal.LinearColor(r=0.15, g=0.15, b=0.15, a=1.0))

    # Add constant for opacity (fully opaque)
    opacity_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 200)
    opacity_node.set_editor_property('r', 1.0)

    # Connect color to base color, opacity to opacity
    mel.connect_material_property(color_node, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(opacity_node, '', unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    eal.save_asset(mat_path)
    print(f'Created deferred decal material: {mat_path}')
else:
    print(f'Using existing material: {mat_path}')

# Now place DecalActors over each WC door sign
# Door01 (men) sign position: roughly center of door, upper half
# Doors face yaw -90, sign is on the +Y face (facing the hallway)
# Door mesh local bounds: X(-10,7), Y(-117,1), Z(4,255)
# Sign is roughly at local Y=0 (front face), Z=160-220 (upper area), X centered

# Remove old cover decals if re-running
for a in all_actors:
    if 'ToiletSignCover' in a.get_actor_label():
        a.destroy_actor()

count = 0
for a in all_actors:
    class_name = a.get_class().get_name()
    if 'BP_WCDoor' not in class_name:
        continue
    label = a.get_actor_label()

    smcs = a.get_components_by_class(unreal.StaticMeshComponent)
    for smc in smcs:
        if not smc.static_mesh or smc.static_mesh.get_name() != 'SM_DoorOffice':
            continue

        comp_loc = smc.get_world_location()
        comp_rot = smc.get_world_rotation()

        # The sign is on the front face of the door
        # Door faces yaw -90 so front face normal is along -Y in world
        # Sign center is roughly at local (0, 0, 190) — upper center of door
        # We need the decal slightly in front of the door face

        # Calculate sign world position
        # With yaw -90: local +Y maps to world -X, local +X maps to world -Y
        # So local Y=0 (front face) is at the component location
        # Local Z=190 maps to world Z+190

        sign_world = unreal.Vector(
            comp_loc.x,  # door face is at component X
            comp_loc.y,  # component Y
            comp_loc.z + 170.0  # sign is in upper portion of door
        )

        # Decal rotation: needs to face into the door surface
        # Door faces yaw -90, so decal should project along +Y (into door front face)
        # DecalActor projects along its -X axis by default
        decal_rot = unreal.Rotator(roll=0, pitch=0, yaw=0)

        decal = subsys.spawn_actor_from_class(unreal.DecalActor, sign_world)
        if decal:
            decal.set_actor_label(f'ToiletSignCover_{label}')
            decal.set_actor_rotation(decal_rot, False)

            # Set decal material and size
            decal_comp = decal.get_component_by_class(unreal.DecalComponent)
            if decal_comp:
                decal_comp.set_decal_material(mat)
                # Decal size is half-extents: X=projection depth, Y=width, Z=height
                decal_comp.set_editor_property('decal_size', unreal.Vector(5, 25, 25))
                print(f'  Placed cover on {label} at ({sign_world.x:.0f}, {sign_world.y:.0f}, {sign_world.z:.0f})')
                count += 1
        else:
            print(f'  FAILED to spawn decal for {label}')

print(f'\nPlaced {count} sign covers')
if count > 0:
    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print('Level saved.')
