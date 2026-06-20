import unreal

mesh = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono.SKM_Operator_Mono")
skel = mesh.get_editor_property("skeleton") if mesh else None
print("skeleton:", skel.get_name() if skel else None)

# List bones whose name hints at a weapon/grip/ik joint, plus the hand chain.
names = []
try:
    bn = skel.get_editor_property("bone_tree") if skel else None
except Exception:
    bn = None

# Reliable path: use the skeletal mesh reference skeleton via the editor lib
try:
    bones = unreal.SkeletalMeshUtilities  # may not exist
except Exception:
    bones = None

# Use the skeleton's get_reference_pose / bone names through the mesh
allnames = []
try:
    num = mesh.get_editor_property("ref_skeleton")  # not exposed; fall back
except Exception:
    pass

# Most reliable: iterate via unreal.SkeletalMesh num bones API
try:
    n = mesh.num_bones if hasattr(mesh, "num_bones") else None
except Exception:
    n = None
print("num_bones attr:", n)

# Try the helper that returns bone names
try:
    bnames = mesh.get_editor_property("skeleton").get_editor_property("bone_tree")
    print("bone_tree len:", len(bnames) if bnames else None)
except Exception as e:
    print("bone_tree err:", e)

# Definitive: unreal.SkeletalMeshComponent path not available here; use anim helper
try:
    import unreal
    helper = unreal.SkeletalMeshEditorSubsystem() if hasattr(unreal, "SkeletalMeshEditorSubsystem") else None
    print("helper:", helper)
except Exception as e:
    print("helper err:", e)

# Print every bone name via the low-level API
try:
    refskel = mesh.get_editor_property("imported_model")
except Exception as e:
    print("imported_model err:", e)

# Simple, works in 5.x: get_bone_index / get_num_bones on the mesh through K2
print("--- bone names via get_socket_names + parent walk not available; dumping via skeleton ---")
try:
    bone_names = []
    skobj = skel
    # UE python: unreal.MeshUtilities? Use this known-good call:
    for i in range(300):
        nm = mesh.get_bone_name(i) if hasattr(mesh, "get_bone_name") else None
        if nm is None:
            break
        bone_names.append(str(nm))
    if bone_names:
        weaponish = [b for b in bone_names if any(k in b.lower() for k in ["weapon","gun","ik_","grip","prop","hold"])]
        handish = [b for b in bone_names if "hand" in b.lower()]
        print("TOTAL:", len(bone_names))
        print("WEAPON/IK:", weaponish)
        print("HAND:", handish)
    else:
        print("get_bone_name not available")
except Exception as e:
    print("bone dump err:", e)
