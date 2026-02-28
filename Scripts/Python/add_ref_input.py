import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
lp = pc.get_local_player()
eis = unreal.EnhancedInputLocalPlayerSubsystem.get_subsystem(lp) if lp else None

if eis:
    imc = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IMC_TacticalShooterPack')
    if imc:
        eis.add_mapping_context(imc, 0)
        unreal.log(f'[FIX] Added IMC_TacticalShooterPack')
    else:
        unreal.log_error('[FIX] Could not load IMC_TacticalShooterPack')
else:
    unreal.log_error('[FIX] No EnhancedInputLocalPlayerSubsystem found')
