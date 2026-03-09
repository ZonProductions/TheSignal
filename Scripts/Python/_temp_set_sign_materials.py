import unreal

for i in range(1, 7):
    bp_path = f"/Game/TheSignal/Blueprints/LevelDesign/BP_FloorSign_{i}"
    mi_path = f"/Game/TheSignal/Materials/MI_FloorSign_{i}"
    bp = unreal.load_asset(bp_path)
    mi = unreal.load_asset(mi_path)

    # Try known UE SCS template naming patterns
    candidates = [
        f"{bp_path}.BP_FloorSign_{i}_C:SignMesh",
        f"{bp_path}.SignMesh_GEN_VARIABLE",
        f"{bp_path}.SignMesh",
        f"BP_FloorSign_{i}.SignMesh_GEN_VARIABLE",
        f"BP_FloorSign_{i}.SignMesh",
    ]

    template = None
    for path in candidates:
        template = unreal.find_object(None, path)
        if template:
            print(f"  Found template at: {path}")
            break

    if template:
        template.set_material(0, mi)
        unreal.EditorAssetLibrary.save_asset(bp_path)
        print(f"BP_FloorSign_{i}: material set")
    else:
        print(f"BP_FloorSign_{i}: template not found, tried {len(candidates)} paths")
