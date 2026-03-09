import unreal

# Place decal covers over the toilet signs on kiosk stall doors.
# F5_kiosk13 is confirmed as SM_ToiletKiosk at (-3558, -1528, 2003) rot (0,0,0)

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Step 1: Create deferred decal material — solid dark grey matching stall color
mat_path = '/Game/TheSignal/Materials/M_StallCover'
mat = unreal.load_asset(mat_path)
if not mat:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset('M_StallCover', '/Game/TheSignal/Materials', unreal.Material, factory)
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)

    # Match the stall door color — greyish white
    color_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, 0)
    color_node.set_editor_property('constant', unreal.LinearColor(r=0.75, g=0.75, b=0.75, a=1.0))

    opacity_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 200)
    opacity_node.set_editor_property('r', 1.0)

    mel.connect_material_property(color_node, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(opacity_node, '', unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    eal.save_asset(mat_path)
    print(f'Created decal material: {mat_path}')
else:
    print(f'Using existing: {mat_path}')

# Step 2: Find all kiosk stalls, get their positions, place decals
all_actors = subsys.get_all_level_actors()

# Remove old covers if re-running
for a in all_actors:
    if 'KioskSignCover' in a.get_actor_label():
        a.destroy_actor()

# Re-fetch after deletions
all_actors = subsys.get_all_level_actors()

count = 0
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    if mesh_name not in ('SM_ToiletKiosk', 'SM_ToiletKioskClosed'):
        continue

    label = a.get_actor_label()
    loc = a.get_actor_location()
    rot = a.get_actor_rotation()

    # The sign is on the front face of the stall door panel
    # Kiosks at rot (0,0,0) face along local Y axis
    # Sign is roughly in the center-upper area of the door panel
    # Place decal at door center, sign height (~130cm up from bottom)
    sign_z = loc.z + 130.0

    # Decal projects along its X axis. For a kiosk facing Y direction,
    # we need the decal to project along Y (into the door face)
    # So rotate decal yaw 90 to align projection with Y
    decal_loc = unreal.Vector(loc.x, loc.y, sign_z)
    decal_rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=rot.yaw + 90.0)

    decal = subsys.spawn_actor_from_class(unreal.DecalActor, decal_loc)
    if decal:
        decal.set_actor_label(f'KioskSignCover_{label}')
        decal.set_actor_rotation(decal_rot, False)
        decal_comp = decal.get_component_by_class(unreal.DecalComponent)
        if decal_comp:
            decal_comp.set_decal_material(mat)
            # X = projection depth, Y = width, Z = height
            decal_comp.set_editor_property('decal_size', unreal.Vector(10, 20, 20))
        print(f'  {label}: decal at ({decal_loc.x:.0f}, {decal_loc.y:.0f}, {decal_loc.z:.0f})')
        count += 1

print(f'\nPlaced {count} stall sign covers')
if count > 0:
    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print('Level saved.')
