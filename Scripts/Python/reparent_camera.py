"""
Reparent Camera_0 to the Player SkeletalMeshComponent at FPCamera socket.
Also configure GASP camera auto-activate for 5.4.
"""
import unreal

bp_path = '/Game/Player/Blueprints/BP_GraceTactical'

# Use SCS (Simple Construction Script) to reparent components
bp = unreal.load_asset(bp_path)
scs = bp.simple_construction_script
if not scs:
    print('ERROR: No SCS found')
else:
    # Find the nodes
    all_nodes = scs.get_all_nodes()
    camera_node = None
    player_node = None
    for node in all_nodes:
        template = node.component_template
        if template:
            name = template.get_name()
            print(f'  SCS Node: {name} ({template.get_class().get_name()})')
            if name == 'Camera_0':
                camera_node = node
            elif name == 'Player':
                player_node = node

    if camera_node and player_node:
        # Reparent camera under player
        camera_node.set_editor_property('parent_component_or_variable_name', 'Player')
        # Set the attach socket
        camera_node.component_template.set_editor_property('attach_socket_name', 'FPCamera')
        # Reset transform
        camera_node.component_template.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
        camera_node.component_template.set_editor_property('relative_rotation', unreal.Rotator(0, 0, 0))
        print('Camera_0 reparented to Player at FPCamera socket')
    else:
        print(f'ERROR: camera_node={camera_node}, player_node={player_node}')

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    print('Saved')
