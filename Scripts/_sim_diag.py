import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
pawn = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.load_asset('/Game/Core/Player/BP_GraceCharacter').generated_class())[0]
mv = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == 'MeleeViewMesh')
kin = next(c for c in pawn.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name())
pipe = next((c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'MeleeViewWeapon'), None)
unreal.log(f'KIN tick enabled: {kin.is_component_tick_enabled()}')
unreal.log(f'MV visible: {mv.is_visible()}  playing: {mv.is_playing()}')
ad = mv.get_editor_property('animation_data')
unreal.log(f'MV anim: {ad.get_editor_property("anim_to_play").get_name() if ad.get_editor_property("anim_to_play") else "NONE"}')
ws = unreal.RelativeTransformSpace.RTS_WORLD
hr = mv.get_socket_transform('hand_r', ws).translation
unreal.log(f'hand_r: ({hr.x:.1f}, {hr.y:.1f}, {hr.z:.1f})')
if pipe:
    pt = pipe.get_world_transform().translation
    unreal.log(f'PIPE: ({pt.x:.1f}, {pt.y:.1f}, {pt.z:.1f})  visible: {pipe.is_visible()}')
else:
    unreal.log('PIPE COMP: None')
unreal.log(f'CurrentWeaponType: {kin.get_editor_property("CurrentWeaponType")}')
