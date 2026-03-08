import unreal

# Check known Character Customizer paths and any head-related meshes
search_paths = [
    '/Game/CharacterCustomizer/',
    '/Game/CC/',
    '/Game/Characters/',
    '/Game/NPC/',
    '/Game/Meshes/',
]

found_any = False
for base_path in search_paths:
    try:
        assets = unreal.EditorAssetLibrary.list_assets(base_path, recursive=True)
    except:
        continue
    for path in assets:
        if path.endswith('_C'):
            continue
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.SkeletalMesh):
            continue
        skel = asset
        ref_skel = skel.get_editor_property('ref_skeleton')
        num_bones = ref_skel.get_num()
        if num_bones < 5:
            continue
        bone_names = [str(ref_skel.get_bone_name(i)) for i in range(num_bones)]
        # Print ALL bones for any mesh with more than 20 bones (likely a character)
        if num_bones > 20:
            print(f"\n=== {skel.get_name()} ({num_bones} bones) | {path} ===")
            for i, name in enumerate(bone_names):
                print(f"  [{i}] {name}")
            found_any = True

if not found_any:
    # Fallback: just find ANY skeletal mesh with "head" in the path/name
    print("Searching all /Game/ for skeletal meshes with 'head' in name...")
    all_assets = unreal.EditorAssetLibrary.list_assets('/Game/', recursive=True)
    for path in all_assets:
        path_lower = path.lower()
        if 'head' in path_lower and not path.endswith('_C'):
            asset = unreal.load_asset(path)
            if isinstance(asset, unreal.SkeletalMesh):
                ref_skel = asset.get_editor_property('ref_skeleton')
                num_bones = ref_skel.get_num()
                bone_names = [str(ref_skel.get_bone_name(i)) for i in range(num_bones)]
                print(f"\n=== {asset.get_name()} ({num_bones} bones) | {path} ===")
                for i, name in enumerate(bone_names):
                    print(f"  [{i}] {name}")
