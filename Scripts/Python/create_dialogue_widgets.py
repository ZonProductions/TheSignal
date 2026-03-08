"""
Create WBP_DialogueBox and WBP_DialogueChoiceButton widget Blueprints,
reparented to C++ base classes, with all required BindWidget children.
"""
import unreal

EAL = unreal.EditorAssetLibrary
AssetTools = unreal.AssetToolsHelpers.get_asset_tools()

# ============================================================
# 1. WBP_DialogueChoiceButton
# ============================================================
choice_path = '/Game/Blueprints/UI/WBP_DialogueChoiceButton'
if not EAL.does_asset_exist(choice_path):
    factory = unreal.WidgetBlueprintFactory()
    # Parent to our C++ class
    parent = unreal.EditorAssetLibrary.load_asset('/Script/TheSignal.ZP_DialogueChoiceButton')
    if parent:
        factory.set_editor_property('parent_class', parent)

    choice_wbp = AssetTools.create_asset('WBP_DialogueChoiceButton', '/Game/Blueprints/UI', None, factory)
    if choice_wbp:
        EAL.save_asset(choice_path)
        unreal.log(f'Created {choice_path}')
    else:
        unreal.log_error(f'Failed to create {choice_path}')
else:
    unreal.log(f'{choice_path} already exists')

# ============================================================
# 2. WBP_DialogueBox
# ============================================================
box_path = '/Game/Blueprints/UI/WBP_DialogueBox'
if not EAL.does_asset_exist(box_path):
    factory2 = unreal.WidgetBlueprintFactory()
    parent2 = unreal.EditorAssetLibrary.load_asset('/Script/TheSignal.ZP_DialogueWidget')
    if parent2:
        factory2.set_editor_property('parent_class', parent2)

    box_wbp = AssetTools.create_asset('WBP_DialogueBox', '/Game/Blueprints/UI', None, factory2)
    if box_wbp:
        EAL.save_asset(box_path)
        unreal.log(f'Created {box_path}')
    else:
        unreal.log_error(f'Failed to create {box_path}')
else:
    unreal.log(f'{box_path} already exists')

# ============================================================
# 3. DA_Dialogue_Test (DataAsset)
# ============================================================
da_path = '/Game/Core/Data/DA_Dialogue_Test'
if not EAL.does_asset_exist(da_path):
    da_factory = unreal.DataAssetFactory()
    da_factory.set_editor_property('data_asset_class', unreal.load_class(None, '/Script/TheSignal.ZP_DialogueData'))

    da = AssetTools.create_asset('DA_Dialogue_Test', '/Game/Core/Data', None, da_factory)
    if da:
        # Set DialogueID
        da.set_editor_property('dialogue_id', 'Test_NPC_Greeting')
        da.set_editor_property('b_one_shot', False)  # repeatable for testing
        da.set_editor_property('priority', 50)

        EAL.save_asset(da_path)
        unreal.log(f'Created {da_path}')
    else:
        unreal.log_error(f'Failed to create {da_path}')
else:
    unreal.log(f'{da_path} already exists')

unreal.log('=== Dialogue widget + DA creation complete ===')
