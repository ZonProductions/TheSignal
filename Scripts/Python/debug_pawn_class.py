import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
if pawn:
    unreal.log(f'[DEBUG] Pawn class: {pawn.get_class().get_name()}')
    ctrl = pawn.get_controller()
    unreal.log(f'[DEBUG] Controller: {ctrl.get_class().get_name() if ctrl else "NULL"}')
else:
    unreal.log_error('[DEBUG] No pawn found')
