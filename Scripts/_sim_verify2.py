"""Phase A (editor world): end any play session, place TEST pawn, start simulate."""
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if les.is_in_play_in_editor():
    les.editor_request_end_play()
    unreal.log('ended previous play session')
else:
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # clean old test pawns
    for a in eas.get_all_level_actors():
        if a.get_actor_label().startswith('TEST_GripPawn'):
            eas.destroy_actor(a)
    bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
    pawn = eas.spawn_actor_from_class(bp.generated_class(),
        unreal.Vector(-1450.0, -950.0, 1090.0), unreal.Rotator(roll=0, pitch=0, yaw=0))
    assert pawn, 'editor spawn failed'
    pawn.set_actor_label('TEST_GripPawn')
    unreal.log(f'PLACED {pawn.get_actor_label()}')
    les.editor_play_simulate()
    unreal.log('SIMULATE REQUESTED')
