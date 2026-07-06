import unreal, json

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LABEL = "Roof_Consolidated_Z3380"
ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"

# --- current consolidated actor + its instanced components ---
actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        actor = a; break
print("Consolidated actor:", actor.get_path_name() if actor else "MISSING")
if actor:
    for c in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
        rs = c.get_relative_scale3d()
        print("  comp '%s' class=%s instances=%d relScale=(%.3f,%.3f,%.3f)" % (
            c.get_name(), c.get_class().get_name(), c.get_instance_count(), rs.x, rs.y, rs.z))
        # actual world transform determinant sign of the component
        wt = c.get_world_transform(); ws = wt.scale3d
        print("     worldScale=(%.3f,%.3f,%.3f)" % (ws.x, ws.y, ws.z))

# --- mesh material + two-sided flag ---
mesh = unreal.load_asset(ROOF_MESH)
smats = mesh.get_editor_property('static_materials')
for i, sm in enumerate(smats):
    mi = sm.get_editor_property('material_interface')
    two_sided = None
    if mi:
        base = mi.get_base_material() if hasattr(mi, 'get_base_material') else mi
        try:
            two_sided = base.get_editor_property('two_sided')
        except Exception as e:
            two_sided = "err:%s" % e
    print("Mesh slot %d material=%s two_sided=%s" % (i, mi.get_path_name() if mi else None, two_sided))

# --- roof XY/Z bounds from backup ---
with open("C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_roof_backup_Z3380.json") as f:
    tiles = json.load(f)["tiles"]
xs = [t["loc"][0] for t in tiles]; ys = [t["loc"][1] for t in tiles]; zs = [t["loc"][2] for t in tiles]
print("Roof X: min=%.1f max=%.1f mid=%.1f" % (min(xs), max(xs), (min(xs)+max(xs))/2))
print("Roof Y: min=%.1f max=%.1f mid=%.1f" % (min(ys), max(ys), (min(ys)+max(ys))/2))
print("Roof Z: min=%.1f max=%.1f" % (min(zs), max(zs)))
print("Span X=%.0f Y=%.0f" % (max(xs)-min(xs), max(ys)-min(ys)))
