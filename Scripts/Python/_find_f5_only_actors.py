"""Find actors that exist on F5 but NOT on F1 — these were deleted by prop cleanup."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Collect F1 and F5 StaticMeshActors with their mesh+material signature
f1_actors = {}  # key = (mesh_name, round(x), round(y)) -> actor
f5_actors = {}

for a in all_actors:
    cls = a.get_class().get_name()
    if cls != 'StaticMeshActor':
        continue
    loc = a.get_actor_location()
    label = a.get_actor_label()

    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property("static_mesh")
    if not sm:
        continue
    mesh_name = sm.get_name()

    # Get materials
    mats = []
    for i in range(smc.get_num_materials()):
        mat = smc.get_material(i)
        if mat:
            mats.append(mat.get_name())

    # F1: Z roughly -50 to 450
    if -50 <= loc.z <= 450:
        key = (mesh_name, round(loc.x / 10) * 10, round(loc.y / 10) * 10)
        f1_actors[key] = (label, loc, mats)

    # F5: Z roughly 1950 to 2450
    elif 1950 <= loc.z <= 2450:
        key = (mesh_name, round(loc.x / 10) * 10, round(loc.y / 10) * 10)
        f5_actors[key] = (label, loc, mesh_name, mats)

# Find F5 actors with NO F1 equivalent
print(f"F1 actors: {len(f1_actors)}, F5 actors: {len(f5_actors)}")
print(f"\n=== F5-ONLY actors (deleted from F1 during prop cleanup) ===")
print(f"=== Filtering to non-furniture (skip bottles, laptops, books, etc) ===\n")

skip_meshes = {'SM_FridgeBottleB', 'SM_LaptopB', 'SM_GlassesSet', 'SM_WaterGlassBottle',
               'SM_BookCaseA', 'SM_Frigde', 'SM_SofaLA_BigTable', 'SM_TableCircle',
               'SM_DeskManagerA', 'SM_Microwave', 'SM_CCTV_CAM', 'SM_CCTV_Base',
               'SM_TVmural', 'SM_Sofa_A_Single', 'SM_FireExtinguisherBoxGlass'}

f5_only = []
for key, (label, loc, mesh, mats) in f5_actors.items():
    if key not in f1_actors and mesh not in skip_meshes:
        f5_only.append((label, loc, mesh, mats))

f5_only.sort(key=lambda x: x[1].x)
for label, loc, mesh, mats in f5_only:
    mat_str = ", ".join(mats)
    has_glass = any('glass' in m.lower() or 'Glass' in m for m in mats)
    marker = " <<< GLASS" if has_glass else ""
    print(f"  {label:50s} | {mesh:30s} | pos=({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f}) | {mat_str}{marker}")

print(f"\nTotal F5-only (non-furniture): {len(f5_only)}")
