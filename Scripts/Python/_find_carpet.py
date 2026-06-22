import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ar = unreal.AssetRegistryHelpers.get_asset_registry()

# 1) find carpet materials in the project
print("=== carpet-named material assets ===")
for cls in ["Material", "MaterialInstanceConstant"]:
    for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", cls)):
        nm = str(ad.asset_name)
        if "carpet" in nm.lower():
            print("  ", nm, "->", str(ad.package_name))

# 2) actors using a carpet material anywhere, with Z + mesh
print("\n=== actors using a 'carpet' material ===")
ztally = {}
mesh_for_carpet = {}
shown = 0
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs:
        continue
    for c in cs:
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m and "carpet" in m.get_name().lower():
                z = a.get_actor_location().z
                b = int(z // 100) * 100
                ztally[b] = ztally.get(b, 0) + 1
                sm = c.get_editor_property("static_mesh")
                mn = sm.get_name() if sm else "?"
                mesh_for_carpet[mn] = mesh_for_carpet.get(mn, 0) + 1
                if shown < 8:
                    org, ext = a.get_actor_bounds(False)
                    print("  %-26s z=%.0f mesh=%-20s mat=%s foot=(%.0fx%.0f) scale=%s" % (
                        a.get_actor_label(), z, mn, m.get_name(), ext.x*2, ext.y*2,
                        (round(a.get_actor_scale3d().x,2),round(a.get_actor_scale3d().y,2))))
                    shown += 1
                break
print("carpet actor Z buckets:", dict(sorted(ztally.items())))
print("carpet meshes:", mesh_for_carpet)

# 3) what materials does SM_SillingTile carry on its actors?
print("\n=== SM_SillingTile material usage (mesh, mats, z) ===")
st = {}
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c = cs[0]
    sm = c.get_editor_property("static_mesh")
    if not sm or sm.get_name() != "SM_SillingTile":
        continue
    mats = tuple(c.get_material(i).get_name() if c.get_material(i) else "none" for i in range(c.get_num_materials()))
    z = int(a.get_actor_location().z // 50) * 50
    st[(mats, z)] = st.get((mats, z), 0) + 1
for k, v in sorted(st.items(), key=lambda kv: -kv[1])[:20]:
    print("  %3d  mats=%s z~%d" % (v, k[0], k[1]))
