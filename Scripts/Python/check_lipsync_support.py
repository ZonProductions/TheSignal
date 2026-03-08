import unreal

# Check CCMH head meshes for morph targets (needed for lip sync)
head_paths = [
    '/Game/CharacterCustomizer/Characters/CCMH/Meshes/',
    '/Game/CharacterCustomizer/Characters/ApparelPack_Example_Character/',
    '/Game/CharacterCustomizer/Characters/CC_MetaHuman_Example/Meshes/',
]

# Also find head meshes by name
registry = unreal.AssetRegistryHelpers.get_asset_registry()
all_skeletal = registry.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine', 'SkeletalMesh'))

unreal.log('=== SCANNING FOR HEAD/FACE SKELETAL MESHES ===')
found_any = False

for asset_data in all_skeletal:
    name = str(asset_data.asset_name)
    path = str(asset_data.package_name)
    if 'head' in name.lower() or 'face' in name.lower() or 'ccmh' in name.lower():
        mesh = unreal.load_asset(str(asset_data.get_full_name()).split(' ')[-1])
        if mesh is None:
            # Try loading by package path
            mesh = unreal.load_asset(path)
        if mesh is None:
            unreal.log(f'\n  SKIP {name} — could not load')
            continue

        found_any = True
        unreal.log(f'\n=== {name} ({path}) ===')

        # Get morph targets
        morphs = mesh.get_editor_property('morph_targets')
        if morphs and len(morphs) > 0:
            unreal.log(f'  MORPH TARGETS ({len(morphs)}):')
            for m in sorted(morphs, key=lambda x: str(x.get_name())):
                unreal.log(f'    {m.get_name()}')
        else:
            unreal.log('  NO MORPH TARGETS')

        # Get skeleton
        skel = mesh.get_editor_property('skeleton')
        if skel:
            unreal.log(f'  Skeleton: {skel.get_path_name()}')

# Also check the CCMH skeleton for jaw/face bones
unreal.log('\n\n=== CHECKING SKELETON FOR JAW/FACE BONES ===')
skel_paths = [
    '/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Skeleton',
    '/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Dummy/CC_Skeleton',
]
for sp in skel_paths:
    skel = unreal.load_asset(sp)
    if skel is None:
        continue
    unreal.log(f'\nSkeleton: {sp}')
    # Get bone names — USkeleton doesn't directly expose bone names easily
    # But we can get the reference skeleton bone count
    # Try via a loaded mesh instead

# Check Head AnimBP
unreal.log('\n\n=== HEAD ANIMBP ===')
head_abp = unreal.load_asset('/Game/CharacterCustomizer/Characters/CCMH/Animation/CCMH_Head_AnimBP')
if head_abp:
    unreal.log(f'  Found: {head_abp.get_path_name()}')
    unreal.log(f'  Class: {head_abp.get_class().get_name()}')
else:
    unreal.log('  NOT FOUND')

# Check Face Physics
face_phys = unreal.load_asset('/Game/CharacterCustomizer/Characters/CCMH/Animation/CCMH_Face_Physics')
if face_phys:
    unreal.log(f'  Found: {face_phys.get_path_name()}')
    unreal.log(f'  Class: {face_phys.get_class().get_name()}')
else:
    unreal.log('  NOT FOUND')

# Check the NPC in the level for what mesh it uses
unreal.log('\n\n=== NPC IN LEVEL ===')
actors = unreal.EditorLevelLibrary.get_all_level_actors()
npcs = [a for a in actors if 'NPC' in a.get_name() or 'npc' in a.get_name().lower()]
for npc in npcs:
    unreal.log(f'\n  {npc.get_name()} ({npc.get_class().get_name()})')
    comps = npc.get_components_by_class(unreal.SkeletalMeshComponent)
    for comp in comps:
        mesh = comp.get_editor_property('skeletal_mesh_asset') if hasattr(comp, 'get_editor_property') else None
        if mesh is None:
            try:
                mesh = comp.skeletal_mesh
            except:
                pass
        if mesh is None:
            try:
                mesh = comp.get_editor_property('skeletal_mesh')
            except:
                pass
        if mesh:
            unreal.log(f'    SkeletalMesh: {mesh.get_path_name()}')
            morphs = mesh.get_editor_property('morph_targets')
            if morphs and len(morphs) > 0:
                unreal.log(f'    Morph targets ({len(morphs)}):')
                for m in sorted(morphs, key=lambda x: str(x.get_name()))[:20]:
                    unreal.log(f'      {m.get_name()}')
                if len(morphs) > 20:
                    unreal.log(f'      ... and {len(morphs) - 20} more')
            else:
                unreal.log('    NO MORPH TARGETS on this mesh')
        else:
            unreal.log(f'    Component: {comp.get_name()} — could not read mesh ref')

if not found_any:
    unreal.log('\n  No head/face meshes found by name. Checking all CCMH meshes...')
    for asset_data in all_skeletal:
        path = str(asset_data.package_name)
        if 'CCMH' in path or 'CharacterCustomizer' in path:
            name = str(asset_data.asset_name)
            unreal.log(f'  Skeletal mesh: {name} — {path}')

unreal.log('\n=== DONE ===')
