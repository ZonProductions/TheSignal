"""One-off read-only census of the currently open level (ResearchFacility).
Dumps placed static-mesh usage, ISM instance counts, actor classes, and Z-bands
to Saved/asset_census/researchfacility_census.json for asset-gap planning.
READ ONLY — mutates nothing, saves nothing."""
import unreal, json, os
from collections import Counter, defaultdict

OUT_DIR = r'C:\Users\Ommei\workspace\TheSignal\Saved\asset_census'
os.makedirs(OUT_DIR, exist_ok=True)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

mesh_counts = Counter()          # mesh asset path -> placed SMA count
mesh_z = defaultdict(list)       # mesh asset path -> actor Z list
class_counts = Counter()         # actor class -> count
ism_instances = Counter()        # mesh asset path -> total ISM/HISM instances
z_hist = Counter()               # 250-UU Z bucket -> actor count
light_classes = Counter()

for a in actors:
    cls = a.get_class().get_name()
    class_counts[cls] += 1
    try:
        z = a.get_actor_location().z
        z_hist[int(z // 250) * 250] += 1
    except Exception:
        z = None
    if isinstance(a, unreal.StaticMeshActor):
        smc = a.static_mesh_component
        sm = smc.static_mesh if smc else None
        p = sm.get_path_name() if sm else 'NONE'
        mesh_counts[p] += 1
        if z is not None:
            mesh_z[p].append(round(z))
    if isinstance(a, unreal.Light) or 'Light' in cls:
        light_classes[cls] += 1
    # ISM/HISM components on any actor (incl. BP_Surface tiles, roof consolidation)
    try:
        for comp in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            sm = comp.static_mesh
            p = sm.get_path_name() if sm else 'NONE'
            ism_instances[p] += comp.get_instance_count()
    except Exception:
        pass

mesh_summary = {}
for p, c in mesh_counts.most_common():
    zs = sorted(mesh_z.get(p, []))
    mesh_summary[p] = {
        'count': c,
        'z_min': zs[0] if zs else None,
        'z_max': zs[-1] if zs else None,
        'z_med': zs[len(zs) // 2] if zs else None,
    }

result = {
    'world': unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name(),
    'total_actors': len(actors),
    'actor_classes': dict(class_counts.most_common()),
    'light_classes': dict(light_classes.most_common()),
    'z_histogram_250uu': {str(k): v for k, v in sorted(z_hist.items())},
    'ism_instances': dict(ism_instances.most_common()),
    'placed_static_meshes': mesh_summary,
}

out_path = os.path.join(OUT_DIR, 'researchfacility_census.json')
with open(out_path, 'w') as f:
    json.dump(result, f, indent=1)
print('CENSUS OK ->', out_path)
print('actors:', len(actors), '| unique meshes:', len(mesh_counts), '| ism mesh types:', len(ism_instances))
