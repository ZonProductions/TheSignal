import unreal

# Duplicate DemoDialogueWidget to our project
src = '/DialoguePlugin/UI/DemoDialogueWidget'
dst = '/Game/Dialogue/WBP_DialogueWidget'

# Check if destination already exists
if unreal.EditorAssetLibrary.does_asset_exist(dst):
    unreal.log('WBP_DialogueWidget already exists, skipping duplicate')
else:
    result = unreal.EditorAssetLibrary.duplicate_asset(src, dst)
    if result:
        unreal.log(f'Duplicated to {dst}')
    else:
        unreal.log(f'FAILED to duplicate {src} to {dst}')

# Also duplicate DemoReplyWidget (child widget used for player choices)
src_reply = '/DialoguePlugin/UI/DemoReplyWidget'
dst_reply = '/Game/Dialogue/WBP_DialogueReplyWidget'
if unreal.EditorAssetLibrary.does_asset_exist(dst_reply):
    unreal.log('WBP_DialogueReplyWidget already exists, skipping')
else:
    result = unreal.EditorAssetLibrary.duplicate_asset(src_reply, dst_reply)
    if result:
        unreal.log(f'Duplicated to {dst_reply}')
    else:
        unreal.log(f'FAILED to duplicate reply widget')

unreal.EditorAssetLibrary.save_directory('/Game/Dialogue/')
unreal.log('Done')
