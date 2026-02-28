import unreal
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
if not bp:
    unreal.log_error('Cannot load BP_GraceCharacter')
else:
    cdo = unreal.get_default_object(bp.generated_class())
    comps = cdo.get_components_by_class(unreal.SceneComponent)
    for c in comps:
        name = c.get_name()
        unreal.log(f'[QUERY] {name} -> RelLoc={c.relative_location}, RelRot={c.relative_rotation}')
        if 'FirstPersonCamera' in name:
            c.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
            c.set_editor_property('relative_rotation', unreal.Rotator(roll=0, pitch=0, yaw=0))
            unreal.log(f'[FIXED] {name} -> (0,0,0)')
        elif 'PlayerMesh' in name:
            c.set_editor_property('relative_location', unreal.Vector(0, 0, -90))
            c.set_editor_property('relative_rotation', unreal.Rotator(roll=0, pitch=0, yaw=-90))
            unreal.log(f'[FIXED] {name} -> RelLoc=(0,0,-90), Yaw=-90')
    unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
    unreal.log('[DONE] BP_GraceCharacter saved')
