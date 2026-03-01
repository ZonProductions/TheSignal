import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
if not bp:
    print("ERROR: Could not load BP_GraceCharacter")
else:
    cdo = unreal.get_default_object(bp.generated_class())

    # Check LocomotionSkeletalMesh
    lsm = cdo.get_editor_property("LocomotionSkeletalMesh")
    print(f"LocomotionSkeletalMesh: {lsm.get_path_name() if lsm else 'NONE'}")

    # Check LocomotionBlendSpace
    lbs = cdo.get_editor_property("LocomotionBlendSpace")
    print(f"LocomotionBlendSpace: {lbs.get_path_name() if lbs else 'NONE'}")

    # Check PlayerMesh AnimClass
    pm = cdo.get_editor_property("PlayerMesh")
    if pm:
        ac = pm.get_editor_property("AnimClass")
        print(f"PlayerMesh.AnimClass: {ac.get_path_name() if ac else 'NONE'}")

        # Check skeletal mesh on PlayerMesh
        sm = pm.get_editor_property("SkeletalMeshAsset")
        print(f"PlayerMesh.SkeletalMesh: {sm.get_path_name() if sm else 'NONE'}")
    else:
        print("PlayerMesh: NONE")

    # Check hidden Mesh
    mesh = cdo.get_editor_property("Mesh")
    if mesh:
        msm = mesh.get_editor_property("SkeletalMeshAsset")
        print(f"HiddenMesh.SkeletalMesh: {msm.get_path_name() if msm else 'NONE'}")
        vis = mesh.get_editor_property("bVisible")
        print(f"HiddenMesh.Visible: {vis}")
        tick_opt = mesh.get_editor_property("VisibilityBasedAnimTickOption")
        print(f"HiddenMesh.TickOption: {tick_opt}")
    else:
        print("Mesh: NONE")
