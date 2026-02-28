import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
cam = [c for c in pawn.get_components_by_class(unreal.CameraComponent) if 'FirstPersonCamera' in c.get_name()][0]

for c in pawn.get_components_by_class(unreal.ActorComponent):
    if c.is_component_tick_enabled():
        c.set_component_tick_enabled(False)

cam.set_editor_property('relative_location', unreal.Vector(0, 0, 0))

for c in pawn.get_components_by_class(unreal.ActorComponent):
    cname = c.get_class().get_name()
    if cname == 'ZP_GraceGameplayComponent':
        c.set_component_tick_enabled(True)
        unreal.log(f'[BISECT] Re-enabled: {cname}')

unreal.log('[BISECT] Camera Z=0, only ZP_GraceGameplayComponent ticking')
