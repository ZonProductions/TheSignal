"""
Configure BP_GraceCharacter, PC_Grace, GM_TheSignal Blueprint CDO defaults.
Sets DataAsset, Input Actions, IMC, and class references.
"""
import unreal

eal = unreal.EditorAssetLibrary

# ── Load all assets ────────────────────────────────────────────────
da_movement = eal.load_asset('/Game/Core/Data/DA_GraceMovement_Default')
ia_move = eal.load_asset('/Game/Core/Input/Actions/IA_Move')
ia_look = eal.load_asset('/Game/Core/Input/Actions/IA_Look')
ia_sprint = eal.load_asset('/Game/Core/Input/Actions/IA_Sprint')
ia_jump = eal.load_asset('/Game/Core/Input/Actions/IA_Jump')
ia_interact = eal.load_asset('/Game/Core/Input/Actions/IA_Interact')
imc_grace = eal.load_asset('/Game/Core/Input/IMC_Grace')

bp_grace = eal.load_asset('/Game/Core/Player/BP_GraceCharacter')
bp_pc = eal.load_asset('/Game/Core/Player/PC_Grace')
bp_gm = eal.load_asset('/Game/Core/Framework/GM_TheSignal')

# Verify all loaded
assets = {
    'DA_GraceMovement_Default': da_movement,
    'IA_Move': ia_move, 'IA_Look': ia_look, 'IA_Sprint': ia_sprint,
    'IA_Jump': ia_jump, 'IA_Interact': ia_interact,
    'IMC_Grace': imc_grace,
    'BP_GraceCharacter': bp_grace, 'PC_Grace': bp_pc, 'GM_TheSignal': bp_gm,
}
for name, asset in assets.items():
    if not asset:
        unreal.log_error(f'Failed to load {name}!')
        raise RuntimeError(f'Asset load failed: {name}')

# ── Helper: Get CDO from Blueprint asset ──────────────────────────
def get_cdo(bp_asset):
    """Get the Class Default Object from a Blueprint asset."""
    gen_class = bp_asset.generated_class()
    return unreal.get_default_object(gen_class)

# ── 1. BP_GraceCharacter CDO ─────────────────────────────────────
cdo_grace = get_cdo(bp_grace)
cdo_grace.set_editor_property('MovementConfig', da_movement)
cdo_grace.set_editor_property('MoveAction', ia_move)
cdo_grace.set_editor_property('LookAction', ia_look)
cdo_grace.set_editor_property('SprintAction', ia_sprint)
cdo_grace.set_editor_property('JumpAction', ia_jump)
cdo_grace.set_editor_property('InteractAction', ia_interact)
unreal.log('[TICKET-005] BP_GraceCharacter CDO configured: MovementConfig + 5 Input Actions')

# ── 2. PC_Grace CDO ──────────────────────────────────────────────
cdo_pc = get_cdo(bp_pc)
cdo_pc.set_editor_property('DefaultMappingContext', imc_grace)
unreal.log('[TICKET-005] PC_Grace CDO configured: DefaultMappingContext = IMC_Grace')

# ── 3. GM_TheSignal CDO ─────────────────────────────────────────
cdo_gm = get_cdo(bp_gm)
# DefaultPawnClass and PlayerControllerClass need class references
bp_grace_class = bp_grace.generated_class()
bp_pc_class = bp_pc.generated_class()
cdo_gm.set_editor_property('DefaultPawnClass', bp_grace_class)
cdo_gm.set_editor_property('PlayerControllerClass', bp_pc_class)
unreal.log('[TICKET-005] GM_TheSignal CDO configured: DefaultPawn=BP_GraceCharacter, Controller=PC_Grace')

# ── Save all Blueprints ──────────────────────────────────────────
eal.save_asset('/Game/Core/Player/BP_GraceCharacter')
eal.save_asset('/Game/Core/Player/PC_Grace')
eal.save_asset('/Game/Core/Framework/GM_TheSignal')
unreal.log('[TICKET-005] All 3 Blueprints saved')

# ── Verify ────────────────────────────────────────────────────────
unreal.log(f'  BP_Grace MovementConfig: {cdo_grace.get_editor_property("MovementConfig")}')
unreal.log(f'  BP_Grace MoveAction: {cdo_grace.get_editor_property("MoveAction")}')
unreal.log(f'  PC_Grace DefaultMappingContext: {cdo_pc.get_editor_property("DefaultMappingContext")}')
unreal.log(f'  GM DefaultPawnClass: {cdo_gm.get_editor_property("DefaultPawnClass")}')
unreal.log(f'  GM PlayerControllerClass: {cdo_gm.get_editor_property("PlayerControllerClass")}')
