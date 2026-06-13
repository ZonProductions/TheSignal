import unreal
at = unreal.AssetToolsHelpers.get_asset_tools()
t = unreal.AssetImportTask()
t.set_editor_property("filename", "C:/Users/Ommei/workspace/TheSignal/Icons/Recolored/Icon_Pipe.png")
t.set_editor_property("destination_path", "/Game/icons")
t.set_editor_property("destination_name", "Icon_Pipe")
t.set_editor_property("automated", True)
t.set_editor_property("save", True)
t.set_editor_property("replace_existing", True)
at.import_asset_tasks([t])

tx = unreal.load_asset("/Game/icons/Icon_Pipe")
tx.set_editor_property("never_stream", True)
tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
tx.set_editor_property("srgb", True)
tx.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_asset("/Game/icons/Icon_Pipe")
print("Icon_Pipe replaced:", tx.blueprint_get_size_x(), "x", tx.blueprint_get_size_y(),
      "never_stream", tx.get_editor_property("never_stream"))
