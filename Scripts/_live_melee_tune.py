import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
if not world:
    unreal.log_error('NO PIE WORLD — is PIE running?')
else:
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        unreal.log_error('NO PAWN')
    else:
        comps = pawn.get_components_by_class(unreal.SkeletalMeshComponent)
        mv = next((c for c in comps if c.get_name() == 'MeleeViewMesh'), None)
        kin = next((c for c in pawn.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name()), None)
        if mv:
            # 1. Hide the view model's head — player was seeing inside the face
            mv.hide_bone_by_name('head', unreal.PhysBodyOp.PBO_NONE)
            unreal.log('HEAD HIDDEN on MeleeViewMesh')
            unreal.log(f'MV rel loc: {mv.get_relative_transform().translation}')
            unreal.log(f'MV rel rot: {mv.get_relative_transform().rotation.rotator()}')
        else:
            unreal.log_error('MeleeViewMesh not found on pawn')
        if kin:
            # 2. Diagnose light/heavy
            heavy = kin.get_editor_property('MeleeHeavyAnim')
            lights = kin.get_editor_property('MeleeLightAnims')
            unreal.log(f'HEAVY: {heavy.get_name() if heavy else "NULL"} len={heavy.get_editor_property("sequence_length") if heavy else 0:.2f}')
            for l in lights:
                unreal.log(f'LIGHT: {l.get_name()} len={l.get_editor_property("sequence_length"):.2f}')
            unreal.log(f'HeavyThreshold: {kin.get_editor_property("MeleeHeavyHoldThreshold")}')
            unreal.log(f'GripOffset: {kin.get_editor_property("MeleeGripOffset")} GripRot: {kin.get_editor_property("MeleeGripRotation")}')
        # 3. Find the pipe weapon comp for grip tuning
        wcomps = pawn.get_components_by_class(unreal.StaticMeshComponent)
        pipe = next((c for c in wcomps if c.get_name() == 'MeleeViewWeapon'), None)
        if pipe:
            unreal.log(f'PIPE comp found. rel loc: {pipe.get_relative_transform().translation} rot: {pipe.get_relative_transform().rotation.rotator()}')
        else:
            unreal.log_error('MeleeViewWeapon comp not found (pipe not equipped yet?)')
