"""
set_bullet_decals.py
Sets UWC Bullet Hole decal materials on BP_GraceCharacter's BulletDecalMaterials array.
Uses a mix of Metal_Solid (textured) and Generic (normal-only, surface-agnostic).
Run via MCP Python: exec(open('Scripts/set_bullet_decals.py').read())
"""
import unreal

bp_path = '/Game/Core/Player/BP_GraceCharacter.BP_GraceCharacter_C'
decal_paths = [
    '/Game/UWC_Bullet_Holes/Instances/Decals/Metal_Solid/MI_Metal_Solid_1.MI_Metal_Solid_1',
    '/Game/UWC_Bullet_Holes/Instances/Decals/Metal_Solid/MI_Metal_Solid_2.MI_Metal_Solid_2',
    '/Game/UWC_Bullet_Holes/Instances/Decals/Metal_Solid/MI_Metal_Solid_3.MI_Metal_Solid_3',
    '/Game/UWC_Bullet_Holes/Instances/Decals/Metal_Solid/MI_Metal_Solid_4.MI_Metal_Solid_4',
    '/Game/UWC_Bullet_Holes/Instances/Decals/Metal_Solid/MI_Metal_Solid_5.MI_Metal_Solid_5',
]

bp = unreal.load_asset(bp_path)
if not bp:
    raise RuntimeError(f'Failed to load BP: {bp_path}')

cdo = unreal.get_default_object(bp)
if not cdo:
    raise RuntimeError(f'Failed to get CDO for: {bp_path}')

materials = []
for path in decal_paths:
    mat = unreal.load_asset(path)
    if mat:
        materials.append(mat)
        print(f'Loaded: {mat.get_name()}')
    else:
        print(f'WARNING: Failed to load {path}')

cdo.set_editor_property('BulletDecalMaterials', materials)
unreal.EditorAssetLibrary.save_asset(bp_path.rsplit('.', 1)[0])
print(f'Set {len(materials)} decal materials on BP_GraceCharacter CDO')
