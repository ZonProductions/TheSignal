import unreal, os

SRC = "C:/Users/Ommei/workspace/TheSignal/Sfx/"
DEST = "/Game/Audio/Weapons"

FILES = [
    ("RANGE_IMPACT_PISTOL.wav",        "SFX_RANGE_IMPACT_PISTOL"),
    ("RANGE_IMPACT_RIFLE.wav",         "SFX_RANGE_IMPACT_RIFLE"),
    ("RANGE_IMPACT_SHOTGUN.wav",       "SFX_RANGE_IMPACT_SHOTGUN"),
    ("MELEE_IMPACT1.wav",              "SFX_MELEE_IMPACT1"),
    ("MELEE_IMPACT2.wav",              "SFX_MELEE_IMPACT2"),
    ("PIPE_HIT.wav",                   "SFX_PIPE_HIT"),
    ("PIPE_SURFACE_WALL_IMPACT.wav",   "SFX_PIPE_SURFACE_WALL_IMPACT"),
]

if not unreal.EditorAssetLibrary.does_directory_exist(DEST):
    unreal.EditorAssetLibrary.make_directory(DEST)

tools = unreal.AssetToolsHelpers.get_asset_tools()
for src_name, asset_name in FILES:
    print("SRC exists:", os.path.exists(SRC + src_name), SRC + src_name)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + src_name)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", asset_name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    t.set_editor_property("factory", unreal.SoundFactory())
    tools.import_asset_tasks([t])
    imported = list(t.get_editor_property("imported_object_paths") or [])
    print("  imported_object_paths:", imported)

for src_name, asset_name in FILES:
    pkg = DEST + "/" + asset_name
    print("EXISTS" if unreal.EditorAssetLibrary.does_asset_exist(pkg) else "MISSING", pkg)
