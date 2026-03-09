import unreal

# Place a DecalActor over the toilet sign on each WC door.
# The decal will be a flat color matching the door surface, covering the sign.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# First, create a simple flat color material for the decal
# Use an existing solid color MI if available
flat_mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_ColorDarkMetallic')
if not flat_mat:
    print('MI_ColorDarkMetallic not found, trying alternatives...')
    flat_mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_DoorBW')

if not flat_mat:
    print('ERROR: No flat material found')
else:
    print(f'Using material: {flat_mat.get_name()}')

    # Find all WC doors and place decals
    count = 0
    for a in all_actors:
        class_name = a.get_class().get_name()
        if 'BP_WCDoor' not in class_name:
            continue
        label = a.get_actor_label()

        # Get the door's world transform
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()

        # Get the door mesh component to find exact position
        smcs = a.get_components_by_class(unreal.StaticMeshComponent)
        for smc in smcs:
            if smc.static_mesh and smc.static_mesh.get_name() == 'SM_DoorOffice':
                # Get world bounds to figure out door face position
                comp_loc = smc.get_world_location()
                comp_rot = smc.get_world_rotation()

                # The sign is roughly in the upper-center of the door
                # Decal needs to be positioned just in front of the door face
                # and sized to cover the sign area

                # Spawn decal component via a DecalActor isn't available
                # Instead, use a simple plane mesh with the flat material
                # placed just in front of the door sign

                print(f'  {label} at ({comp_loc.x:.0f}, {comp_loc.y:.0f}, {comp_loc.z:.0f}) rot=({comp_rot.pitch:.0f}, {comp_rot.yaw:.0f}, {comp_rot.roll:.0f})')
                count += 1

    print(f'\nFound {count} WC doors')
    print('Need door dimensions and sign location to place covers...')

    # Get the mesh bounds to understand door geometry
    if count > 0:
        for a in all_actors:
            if a.get_actor_label() == 'BP_WCDoor01':
                smcs = a.get_components_by_class(unreal.StaticMeshComponent)
                for smc in smcs:
                    if smc.static_mesh and smc.static_mesh.get_name() == 'SM_DoorOffice':
                        bounds = smc.get_local_bounds()
                        print(f'\nDoor mesh bounds:')
                        print(f'  Min: ({bounds[0].x:.1f}, {bounds[0].y:.1f}, {bounds[0].z:.1f})')
                        print(f'  Max: ({bounds[1].x:.1f}, {bounds[1].y:.1f}, {bounds[1].z:.1f})')
                        # World bounds
                        origin = unreal.Vector()
                        extent = unreal.Vector()
                        smc.get_local_bounds(origin, extent)
                break
