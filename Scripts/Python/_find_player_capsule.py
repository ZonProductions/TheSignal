"""Find Grace's character class capsule dimensions."""
import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()

# Hunt for the player character BP by common names
candidates = [
    'ZP_GraceCharacter', 'BP_Grace', 'BP_GraceCharacter', 'BP_PlayerCharacter',
]
search_paths = ['/Game/Blueprints', '/Game/ZonPortal', '/Game/Character', '/Game']
found = None
for name in candidates:
    for sp in search_paths:
        path = f'{sp}/{name}.{name}'
        data = ar.get_asset_by_object_path(path)
        if data and data.is_valid():
            found = (name, sp, data)
            break
    if found:
        break

if not found:
    # Brute force: scan for any BP whose class is Character
    print("Direct lookup failed — scanning /Game for Character-based BPs…")
    bps = ar.get_assets_by_path('/Game', recursive=True)
    for d in bps:
        if d.asset_class_path.asset_name != 'Blueprint':
            continue
        nm = str(d.asset_name)
        if 'Grace' in nm or ('Character' in nm and 'BP' in nm.split('_')[0:1]):
            print(f"  candidate: {d.package_name}")
            try:
                loaded = unreal.EditorAssetLibrary.load_asset(str(d.package_name))
                if loaded:
                    gen_cls = loaded.generated_class()
                    cdo = unreal.get_default_object(gen_cls)
                    cap = cdo.get_component_by_class(unreal.CapsuleComponent)
                    if cap:
                        print(f"    Capsule: r={cap.get_unscaled_capsule_radius():.1f}, h={cap.get_unscaled_capsule_half_height():.1f}")
            except Exception as e:
                print(f"    err: {e}")
else:
    name, sp, data = found
    print(f"FOUND: {sp}/{name}")
    loaded = unreal.EditorAssetLibrary.load_asset(f'{sp}/{name}')
    if loaded:
        gen_cls = loaded.generated_class()
        cdo = unreal.get_default_object(gen_cls)
        cap = cdo.get_component_by_class(unreal.CapsuleComponent)
        if cap:
            r = cap.get_unscaled_capsule_radius()
            h = cap.get_unscaled_capsule_half_height()
            print(f"Capsule: radius={r:.1f} half_height={h:.1f}  diameter={2*r:.1f} total_height={2*h:.1f}")

# Also — find the actually-spawned player pawn in the level if any
print("\nSpawned pawns in level:")
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in sub.get_all_level_actors():
    if isinstance(a, unreal.Character) or isinstance(a, unreal.Pawn):
        cap = a.get_component_by_class(unreal.CapsuleComponent)
        if cap:
            print(f"  {a.get_actor_label()} [{type(a).__name__}] r={cap.get_unscaled_capsule_radius():.1f} h={cap.get_unscaled_capsule_half_height():.1f}")
        else:
            print(f"  {a.get_actor_label()} [{type(a).__name__}] (no capsule)")

print("\nDone.")
