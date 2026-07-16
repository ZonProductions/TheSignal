# create_bp_oozeling.py — create /Game/Enemies/Oozeling/BP_Oozeling as a fresh child of
# AZP_OozelingBase and point its inherited Character Mesh at the BigBlob skeletal mesh.
#
# Fresh-child + CDO-mutate pattern (create_bp_floorsign.py lineage). NEVER delete+recreate:
# if the BP already exists this script mutates it in place (feedback_no_python_asset_overwrite).
# SCS components (DeathSave / EasySaveGameComp / BloodFX) are added separately via BlueprintMCP.
#
# Run via the MCP Python endpoint:
#   curl -s -X POST http://localhost:9847/api/python -H "Content-Type: application/json" \
#        -d "{\"file\": \"C:/Users/Ommei/workspace/TheSignal/Scripts/Python/create_bp_oozeling.py\"}"

import unreal

BP_DIR = '/Game/Enemies/Oozeling'
BP_NAME = 'BP_Oozeling'
BP_PATH = BP_DIR + '/' + BP_NAME
MESH_PATH = '/Game/BigBlob/Mesh/SKM_BigBlob'
PARENT = '/Script/TheSignal.ZP_OozelingBase'

if unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
    bp = unreal.load_asset(BP_PATH)
    print('EXISTS (mutating in place): %s' % BP_PATH)
else:
    parent_class = unreal.load_class(None, PARENT)
    if not parent_class:
        raise RuntimeError('Parent class not found: %s — was the C++ build loaded?' % PARENT)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', parent_class)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        BP_NAME, BP_DIR, unreal.Blueprint, factory)
    if not bp:
        raise RuntimeError('create_asset failed for %s' % BP_PATH)
    print('CREATED: %s' % BP_PATH)

# Point the inherited Character Mesh (native component, configured in the C++ ctor) at SKM_BigBlob.
gen_class = unreal.load_class(None, BP_PATH + '.' + BP_NAME + '_C')
cdo = unreal.get_default_object(gen_class)
mesh_comp = cdo.get_editor_property('mesh')
skm = unreal.load_asset(MESH_PATH)
if not skm:
    raise RuntimeError('SkeletalMesh not found: %s' % MESH_PATH)
try:
    mesh_comp.set_editor_property('skeletal_mesh_asset', skm)   # UE5 name
except Exception:
    mesh_comp.set_editor_property('skeletal_mesh', skm)         # legacy fallback
print('MESH SET: %s' % str(mesh_comp.get_editor_property('skeletal_mesh_asset')))

# Compile + save so the CDO change is serialized into the asset.
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    print('COMPILED')
except Exception as e:
    print('compile_blueprint unavailable/failed: %s' % e)
print('SAVED: %s' % unreal.EditorAssetLibrary.save_asset(BP_PATH))
