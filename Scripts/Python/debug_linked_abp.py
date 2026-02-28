import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

player_mesh = None
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    if 'PlayerMesh' in c.get_name():
        player_mesh = c
        break

anim = player_mesh.get_anim_instance()
unreal.log(f'[DEBUG] Parent AnimBP: {anim.get_class().get_name()}')
unreal.log(f'[DEBUG] Parent Pitch: {anim.get_editor_property("Pitch"):.1f}')

linked = player_mesh.get_linked_anim_instances()
unreal.log(f'[DEBUG] Linked AnimBP count: {len(linked)}')
for i, la in enumerate(linked):
    cname = la.get_class().get_name()
    unreal.log(f'[DEBUG] Linked[{i}]: {cname}')
    try:
        p = la.get_editor_property('Pitch')
        unreal.log(f'[DEBUG]   Pitch = {p:.1f}')
    except:
        unreal.log(f'[DEBUG]   (no Pitch var)')
    try:
        ch = la.get_editor_property('Character')
        unreal.log(f'[DEBUG]   Character = {ch.get_name() if ch else "NULL"}')
    except:
        unreal.log(f'[DEBUG]   (no Character var)')
    try:
        sc = la.get_editor_property('TacticalShooterComponent')
        unreal.log(f'[DEBUG]   ShooterComp = {sc.get_class().get_name() if sc else "NULL"}')
    except:
        pass
    try:
        iw = la.get_editor_property('IkWeight')
        unreal.log(f'[DEBUG]   IkWeight = {iw:.2f}')
    except:
        pass
