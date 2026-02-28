"""
setup_kinemation_camera.py

Adds AC_FirstPersonCamera component to BP_GraceCharacter SCS
and sets PlayerMesh skeletal mesh to SKM_Operator_Mono.

Run via MCP Python endpoint: exec(open('Scripts/Python/setup_kinemation_camera.py').read())
"""

import unreal

# --- Paths ---
BP_PATH = '/Game/Core/Player/BP_GraceCharacter'
AC_FPC_PATH = '/Game/KINEMATION/Common/FirstPersonCamera/AC_FirstPersonCamera'
SKM_OPERATOR_PATH = '/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono'

# --- Load assets ---
bp_asset = unreal.load_asset(BP_PATH)
if not bp_asset:
    raise RuntimeError(f'Failed to load BP: {BP_PATH}')

ac_fpc_class = unreal.load_class(None, AC_FPC_PATH + '_C')
if not ac_fpc_class:
    # Try loading as Blueprint and getting generated class
    ac_fpc_bp = unreal.load_asset(AC_FPC_PATH)
    if ac_fpc_bp:
        ac_fpc_class = ac_fpc_bp.generated_class()
    if not ac_fpc_class:
        raise RuntimeError(f'Failed to load AC_FirstPersonCamera class from: {AC_FPC_PATH}')

skm_operator = unreal.load_asset(SKM_OPERATOR_PATH)
if not skm_operator:
    raise RuntimeError(f'Failed to load skeletal mesh: {SKM_OPERATOR_PATH}')

print(f'[Setup] BP: {bp_asset.get_name()}')
print(f'[Setup] AC_FPC class: {ac_fpc_class.get_name()}')
print(f'[Setup] SKM: {skm_operator.get_name()}')

# --- Step 1: Add AC_FirstPersonCamera to SCS ---
scs = bp_asset.simple_construction_script
if not scs:
    raise RuntimeError('BP has no SCS!')

# Check if AC_FirstPersonCamera already exists
existing_fpc = None
for node in scs.get_all_nodes():
    template = node.component_template
    if template and template.get_class().get_name() == ac_fpc_class.get_name():
        existing_fpc = node
        print(f'[Setup] AC_FirstPersonCamera already exists in SCS: {template.get_name()}')
        break

if not existing_fpc:
    new_node = scs.create_node(ac_fpc_class, 'AC_FirstPersonCamera')
    if new_node:
        print(f'[Setup] Added AC_FirstPersonCamera to SCS')
    else:
        raise RuntimeError('Failed to create AC_FirstPersonCamera SCS node')

# --- Step 2: Set PlayerMesh skeletal mesh ---
# PlayerMesh is created in C++ constructor, so it's on the CDO as a default subobject
gen_class = bp_asset.generated_class()
cdo = unreal.get_default_object(gen_class)

player_mesh = cdo.get_editor_property('PlayerMesh')
if player_mesh:
    player_mesh.set_editor_property('SkeletalMesh', skm_operator)
    print(f'[Setup] Set PlayerMesh skeletal mesh to: {skm_operator.get_name()}')
else:
    # Try finding it in SCS nodes
    found = False
    for node in scs.get_all_nodes():
        template = node.component_template
        if template and template.get_name() == 'PlayerMesh':
            template.set_editor_property('SkeletalMesh', skm_operator)
            print(f'[Setup] Set PlayerMesh skeletal mesh via SCS template to: {skm_operator.get_name()}')
            found = True
            break
    if not found:
        print('[WARN] Could not find PlayerMesh component to set skeletal mesh')

# --- Step 3: Save ---
unreal.EditorAssetLibrary.save_asset(BP_PATH)
print(f'[Setup] Saved {BP_PATH}')
print('[Setup] DONE - Open editor, PIE test: arms visible, peek still works')
