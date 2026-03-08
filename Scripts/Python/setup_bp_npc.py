import unreal

bp = unreal.load_asset('/Game/Blueprints/Actors/BP_NPC')
if not bp:
    raise Exception('BP_NPC not found')

# Find InteractionVolume on the CDO and set collision profile
gen_class = bp.generated_class()
cdo = gen_class.get_default_object()

# Iterate CDO components to find InteractionVolume
for prop_name in dir(cdo):
    if 'InteractionVolume' in prop_name:
        print(f'Found property: {prop_name}')

# Try getting it as a component via the SCS nodes
scs = getattr(bp, 'SimpleConstructionScript', None)
print(f'SCS via attr: {scs}')

# Try blueprint subsystem approach
editor_subsystem = unreal.get_editor_subsystem(unreal.SubobjectDataSubsystem)
print(f'SubobjectDataSubsystem: {editor_subsystem}')
