"""Re-import the weapon icons from 32-bit TGA (UE's most reliable UI alpha
format), replacing the PNG-sourced textures at the same /Game/icons paths.
Same UI settings as Icon_Pistol (TC_EDITOR_ICON, sRGB, no mips)."""
import unreal
at = unreal.AssetToolsHelpers.get_asset_tools()
BASE = "C:/Users/Ommei/workspace/TheSignal/Icons/Recolored/TGA/"
NAMES = ["Icon_Rifle", "Icon_Shotgun", "Icon_Pipe", "Icon_Grenade"]

tasks = []
for name in NAMES:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", BASE + name + ".tga")
    t.set_editor_property("destination_path", "/Game/icons")
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    tasks.append(t)
at.import_asset_tasks(tasks)

for name in NAMES:
    p = "/Game/icons/" + name
    tx = unreal.load_asset(p)
    if isinstance(tx, unreal.Texture2D):
        tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tx.set_editor_property("srgb", True)
        tx.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        unreal.EditorAssetLibrary.save_asset(p)
        print("TGA OK %-14s %sx%s" % (name, tx.blueprint_get_size_x(), tx.blueprint_get_size_y()))
    else:
        print("FAILED:", name)
