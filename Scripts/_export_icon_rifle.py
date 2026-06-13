import unreal, os
out_dir = "C:/Users/Ommei/workspace/TheSignal/Icons/_ue_export"
os.makedirs(out_dir, exist_ok=True)
at = unreal.AssetToolsHelpers.get_asset_tools()
at.export_assets(["/Game/icons/Icon_Rifle", "/Game/icons/Icon_Pistol"], out_dir)
print("exported to", out_dir)
