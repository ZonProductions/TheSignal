import unreal

paths = [
    "/Game/Core/Items/DA_Letter_Test",
    "/Game/Core/Items/DA_SecurityLog",
]

for path in paths:
    da = unreal.load_asset(path)
    if not da:
        print(f"NOT FOUND: {path}")
        continue
    tags = da.get_editor_property("GameplayTags")
    tags.import_text('(GameplayTags=("Item.Note"))')
    da.set_editor_property("GameplayTags", tags)
    unreal.EditorAssetLibrary.save_asset(path)
    print(f"{path}: tag applied, saved=True")
