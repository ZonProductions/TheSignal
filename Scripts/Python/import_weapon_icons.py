"""Import the recolored weapon icons into /Game/icons with the same UI texture
settings as the working Icon_Pistol (TC_EDITOR_ICON, sRGB, no mips).
Mapping: Rifle reuses the bullets art (like Pistol)."""
import unreal
at = unreal.AssetToolsHelpers.get_asset_tools()
BASE = "C:/Users/Ommei/workspace/TheSignal/Icons/Recolored/"
JOBS = [
    ("DA_AssualtRifle.png", "Icon_Rifle"),    # bullets (same as pistol)
    ("DA_PolicShotgun.png", "Icon_Shotgun"),
    ("DA_Pipe.png",         "Icon_Pipe"),
    ("DA_Explosive.png",    "Icon_Grenade"),
]

tasks = []
for fn, name in JOBS:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", BASE + fn)
    t.set_editor_property("destination_path", "/Game/icons")
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    tasks.append(t)
at.import_asset_tasks(tasks)

for fn, name in JOBS:
    p = "/Game/icons/" + name
    tx = unreal.load_asset(p)
    if isinstance(tx, unreal.Texture2D):
        tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tx.set_editor_property("srgb", True)
        tx.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_asset(p)
        print("OK %-14s %sx%s" % (name, tx.blueprint_get_size_x(), tx.blueprint_get_size_y()))
    else:
        print("FAILED:", name)
