import unreal

# The DemoDialogueWidget_C's parent should be DialogueUserWidget
bp = unreal.load_asset('/DialoguePlugin/UI/DemoDialogueWidget')
gen = bp.generated_class()

# Walk class hierarchy via super_struct
unreal.log('=== Full class hierarchy ===')
cls = gen
depth = 0
while cls is not None:
    unreal.log(f'  {"  " * depth}{cls.get_name()} ({cls.get_path_name()})')
    try:
        parent = cls.get_editor_property('super_struct')
        cls = parent
    except:
        break
    depth += 1

# Try getting the parent class CDO directly
# The gen class's parent should be DialogueUserWidget
parent_path = gen.get_path_name()
unreal.log(f'Gen path: {parent_path}')

# Check if NPCActor has a Dialogue property we can use
# The plugin might expect NPCActor to have a "Dialogue" property
# Let's check our BP_NPC
bp_npc = unreal.load_asset('/Game/Blueprints/BP_NPC')
if bp_npc:
    npc_gen = bp_npc.generated_class()
    npc_cdo = unreal.get_default_object(npc_gen)
    unreal.log('=== BP_NPC properties with dialogue ===')
    for field in sorted(dir(npc_cdo)):
        if 'dialogue' in field.lower() or 'dialog' in field.lower() or 'plugin' in field.lower():
            try:
                val = npc_cdo.get_editor_property(field)
                unreal.log(f'  {field} = {val}')
            except:
                unreal.log(f'  {field} (get failed)')
else:
    unreal.log('BP_NPC not found at /Game/Blueprints/BP_NPC')

# Try other paths
for path in ['/Game/Blueprints/NPC/BP_NPC', '/Game/Characters/BP_NPC', '/Game/NPC/BP_NPC']:
    bp_npc = unreal.load_asset(path)
    if bp_npc:
        unreal.log(f'Found NPC at: {path}')
        break
