import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
comps = pawn.get_components_by_class(unreal.CameraComponent)
for c in comps:
    if 'FirstPersonCamera' in c.get_name():
        before = c.relative_location
        unreal.log(f'[FORCE] BEFORE RelLoc: Z={before.z:.1f}')
        c.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
        after = c.relative_location
        unreal.log(f'[FORCE] AFTER  RelLoc: Z={after.z:.1f}')
        unreal.log(f'[FORCE] WorldLoc Z: {c.get_world_location().z:.1f}, Capsule Z: {pawn.get_actor_location().z:.1f}')
