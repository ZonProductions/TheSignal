import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn() if world else None
if not pawn:
    print("NO PIE"); 
else:
    for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
        n=c.get_name()
        if n in ("MarcusBody","PlayerMesh","CharacterMesh0"):
            try: uro = c.get_editor_property("enable_update_rate_optimizations")
            except Exception as e: uro="?(%s)"%e
            try: vto = c.get_editor_property("visibility_based_anim_tick_option")
            except Exception as e: vto="?"
            print(n, "URO=", uro, "VisTickOpt=", vto, "rendered=", c.was_recently_rendered(0.1) if hasattr(c,'was_recently_rendered') else '?')
