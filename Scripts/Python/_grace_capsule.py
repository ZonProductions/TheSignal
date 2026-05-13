import unreal
for path in ("/Game/Core/Player/BP_GraceCharacter.BP_GraceCharacter",
             "/Game/Core/Player/BP_GraceCharacter",
             "Blueprint'/Game/Core/Player/BP_GraceCharacter.BP_GraceCharacter'"):
    bp = unreal.EditorAssetLibrary.load_asset(path)
    print(f"try {path!r} -> {bp}")
    if bp:
        gc = bp.generated_class()
        cdo = unreal.get_default_object(gc)
        cap = cdo.get_component_by_class(unreal.CapsuleComponent)
        if cap:
            r = cap.get_unscaled_capsule_radius()
            h = cap.get_unscaled_capsule_half_height()
            print(f"GRACE CAPSULE: radius={r} half_height={h}")
        else:
            # Walk components
            for c in cdo.get_components_by_class(unreal.SceneComponent):
                print(f"  comp {c.get_name()} [{type(c).__name__}]")
        break
