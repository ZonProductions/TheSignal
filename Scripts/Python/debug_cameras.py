"""Debug: List all components accessible on BP_GraceTactical CDO."""
import unreal

bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)

# Try direct property access for inherited components
for attr in ['camera_component', 'spring_arm', 'camera', 'Camera', 'SpringArm']:
    if hasattr(cdo, attr):
        val = getattr(cdo, attr)
        print(f'  {attr} = {val}')

# Check the SCS for all component templates
bp = unreal.load_asset('/Game/Player/Blueprints/BP_GraceTactical')
scs = bp.simple_construction_script
if scs:
    nodes = scs.get_all_nodes()
    print(f'SCS nodes: {len(nodes)}')
    for node in nodes:
        t = node.component_template
        if t:
            print(f'  SCS: {t.get_name()} ({t.get_class().get_name()}) auto_activate={t.get_editor_property("auto_activate")}')

# Try parent class SCS
parent_bp = unreal.load_asset('/Game/Blueprints/CBP_SandboxCharacter')
if parent_bp:
    pscs = parent_bp.simple_construction_script
    if pscs:
        pnodes = pscs.get_all_nodes()
        print(f'Parent SCS nodes: {len(pnodes)}')
        for node in pnodes:
            t = node.component_template
            if t:
                print(f'  Parent SCS: {t.get_name()} ({t.get_class().get_name()}) auto_activate={t.get_editor_property("auto_activate")}')
