"""Tag DA_Letter_Test with Item.Note GameplayTag and save."""
import unreal

asset = unreal.load_asset('/Game/Core/Items/DA_Letter_Test')
if not asset:
    print('ERROR: DA_Letter_Test not found')
    raise SystemExit

# Get the GameplayTagContainer and add Item.Note tag
tags = asset.get_editor_property('GameplayTags')
tags.import_text('(GameplayTags=((TagName="Item.Note")))')

# Write back (import_text modifies in place on the CDO, but set it explicitly too)
asset.set_editor_property('GameplayTags', tags)

# Verify
tags2 = asset.get_editor_property('GameplayTags')
exported = tags2.export_text()
print(f'Tags after set: {exported}')

# Save the asset
saved = unreal.EditorAssetLibrary.save_asset('/Game/Core/Items/DA_Letter_Test')
print(f'Saved: {saved}')
