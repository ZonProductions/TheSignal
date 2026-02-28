import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

cam = [c for c in pawn.get_components_by_class(unreal.CameraComponent) if 'FirstPersonCamera' in c.get_name()][0]

disabled = []
for c in pawn.get_components_by_class(unreal.ActorComponent):
    name = c.get_class().get_name()
    if c.is_component_tick_enabled():
        c.set_component_tick_enabled(False)
        disabled.append(name)

unreal.log(f'[ISOLATE] Disabled ticks on: {disabled}')

cam.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
unreal.log(f'[ISOLATE] Camera Z forced to 0')
