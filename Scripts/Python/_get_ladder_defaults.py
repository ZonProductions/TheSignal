import unreal

# Get BP_MasterLadder defaults
bp = unreal.load_asset('/Game/LadderClimbingSystem/Blueprints/AdjustableLadders/BP_MasterLadder')
cdo = unreal.get_default_object(bp.generated_class())
print('=== BP_MasterLadder Defaults ===')
print('FootBarSpread:', cdo.get_editor_property('FootBarSpread'))
print('SideDistance:', cdo.get_editor_property('SideDistance'))
print('LadderLength:', cdo.get_editor_property('LadderLength'))
print('bSyncedFootBar:', cdo.get_editor_property('bSyncedFootBar'))

# Check each child BP for variable overrides
for i in range(1, 6):
    name = f'BP_Ladder{i}'
    path = f'/Game/LadderClimbingSystem/Blueprints/AdjustableLadders/{name}'
    child_bp = unreal.load_asset(path)
    if child_bp:
        child_cdo = unreal.get_default_object(child_bp.generated_class())
        print(f'\n=== {name} ===')
        print('FootBarSpread:', child_cdo.get_editor_property('FootBarSpread'))
        print('SideDistance:', child_cdo.get_editor_property('SideDistance'))
        print('LadderLength:', child_cdo.get_editor_property('LadderLength'))
        print('bSyncedFootBar:', child_cdo.get_editor_property('bSyncedFootBar'))
    else:
        print(f'\n{name}: NOT FOUND')
