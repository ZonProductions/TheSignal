"""Check GASP-related assets exist in the project."""
import unreal

eal = unreal.EditorAssetLibrary

# Check UEFN Mannequin mesh
paths_to_check = [
    ('/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin', 'SKM_UEFN_Mannequin'),
    ('/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin', 'SKM_UEFN_Mannequin (gasp)'),
    ('/Game/gasp_Blueprints/CBP_SandboxCharacter', 'CBP_SandboxCharacter'),
    ('/Game/gasp_Blueprints/ABP_SandboxCharacter', 'ABP_SandboxCharacter'),
]

for path, label in paths_to_check:
    exists = eal.does_asset_exist(path)
    unreal.log(f'{label}: {"EXISTS" if exists else "NOT FOUND"} at {path}')

# Search for RTG retargeter
import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
results = ar.get_assets_by_class(top_level_only=False, class_path_name='/Script/IKRig.IKRetargeter')
for r in results:
    name = str(r.get_editor_property('asset_name'))
    pkg = str(r.get_editor_property('package_name'))
    if 'UEFN' in name or 'UE5' in name or 'Mannequin' in name or 'RTG' in name:
        unreal.log(f'Retargeter: {name} at {pkg}')
