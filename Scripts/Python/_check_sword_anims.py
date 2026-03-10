import unreal

# Check PlayerMesh skeleton
grace_bp = unreal.load_asset('/Game/TheSignal/Blueprints/BP_GraceCharacter')
print("=== GRACE CHARACTER ===")
if grace_bp:
    cdo = unreal.get_default_object(grace_bp.generated_class())
    for comp in cdo.get_components_by_class(unreal.SkeletalMeshComponent):
        name = comp.get_name()
        mesh = comp.get_editor_property('skeletal_mesh_asset') if hasattr(comp, 'skeletal_mesh_asset') else None
        if mesh is None:
            try:
                mesh = comp.get_editor_property('skeletal_mesh')
            except:
                pass
        if mesh:
            skel = mesh.get_editor_property('skeleton')
            print(f"  {name}: mesh={mesh.get_name()}, skeleton={skel.get_name() if skel else 'NONE'}")
        else:
            print(f"  {name}: mesh=NONE")

# Check sword animation packs
print("\n=== SWORD ANIMATION PACK 1 ===")
anim1 = unreal.load_asset('/Game/Sword_Animation/Animations/anim_attack_light_02')
if anim1:
    skel = anim1.get_editor_property('skeleton') if hasattr(anim1, 'skeleton') else None
    if skel is None:
        try:
            # For AnimSequences, skeleton is accessed via get_skeleton()
            skel = anim1.get_skeleton() if hasattr(anim1, 'get_skeleton') else None
        except:
            pass
    print(f"  anim_attack_light_02: class={anim1.get_class().get_name()}, skeleton={skel.get_name() if skel else 'CHECK MANUALLY'}")
    print(f"  Length: {anim1.get_editor_property('sequence_length') if hasattr(anim1, 'sequence_length') else 'N/A'}")
else:
    print("  NOT FOUND")

montage1 = unreal.load_asset('/Game/Sword_Animation/Animations/anim_attack_light_02_Montage')
if montage1:
    print(f"  Montage: class={montage1.get_class().get_name()}")
else:
    print("  Montage: NOT FOUND")

print("\n=== INVENTORY SYSTEM PRO SWORD ANIMS ===")
for name in ['Attack_PrimaryA', 'Attack_PrimaryB', 'Attack_PrimaryC', 'Sword_Idle']:
    path = f'/Game/InventorySystemPro/ExampleContent/Common/Animations/Sword/{name}'
    asset = unreal.load_asset(path)
    if asset:
        skel = None
        try:
            skel = asset.get_skeleton()
        except:
            pass
        print(f"  {name}: class={asset.get_class().get_name()}, skeleton={skel.get_name() if skel else 'CHECK'}")
    else:
        print(f"  {name}: NOT FOUND")

# Check montages
for name in ['Attack_PrimaryA_Montage', 'Attack_PrimaryB_Montage', 'Attack_PrimaryC_Montage']:
    path = f'/Game/InventorySystemPro/ExampleContent/Common/Animations/Sword/{name}'
    asset = unreal.load_asset(path)
    if asset:
        print(f"  {name}: class={asset.get_class().get_name()}")
    else:
        print(f"  {name}: NOT FOUND")

print("\n=== EXISTING FPS ANIMS (for skeleton reference) ===")
fps_anim = unreal.load_asset('/Game/Animations/FPS/A_FP_PipeSwing')
if fps_anim:
    skel = None
    try:
        skel = fps_anim.get_skeleton()
    except:
        pass
    print(f"  A_FP_PipeSwing: skeleton={skel.get_name() if skel else 'CHECK'}")
else:
    print("  A_FP_PipeSwing: NOT FOUND")

print("\n=== DONE ===")
