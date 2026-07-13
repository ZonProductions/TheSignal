# apply_viscut_rule.py — REUSABLE ACROSS LEVELS
#
# RULE: any actor tagged 'VisCut' defines a visibility-cut volume. Consolidated
# floor/roof ISM slabs (one massive unit per level) become invisible within the
# combined borders of each tagged cluster — per-pixel, via a world-space box
# clip in a masked variant of the slab's own material. Geometry, collision and
# the consolidation itself are untouched.
#
# USAGE (any level): tag the enclosing actors (elevator shaft shells, stair
# runs) with 'VisCut' — or let SEED do it — then run this script via the MCP
# Python endpoint. Rerun any time actors move; boxes recompute from live bounds.
# Saves ONLY material assets. NEVER saves the level.
#
# SEED (idempotent): auto-tags (a) SM_Elevator_Wall* actors that are closed
# shells (XY extents both >= SHELL_MIN_EXTENT — straight lobby segments are
# thin and skipped), (b) every SM_Stair* actor. Add/remove 'VisCut' tags by
# hand for anything else and rerun.
import unreal

TAG = 'VisCut'
SEED = True
SHELL_MIN_EXTENT = 150.0
TARGET_LABEL_PREFIXES = ('Floor_Consolidated',)
MAT_DIR = '/Game/TheSignal/Materials/VisCut'
MAX_BOXES = 8
MERGE_GAP = 60.0        # clusters merge when boxes inflated by this touch
INSET_XY = 15.0         # clip seam hides inside the wall/stair shell
INSET_Z_BOTTOM = 40.0   # keep pit/approach floor under the volume
EXPAND_Z_TOP = 25.0     # catch the slab plane a stair run emerges through

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
# No editor world (mid map-transition) → get_all_level_actors() hard-asserts in C++.
# The watcher guards this too; this keeps a MANUAL run from crashing the editor.
_world = ues.get_editor_world() if ues else None
if not _world or not unreal.SystemLibrary.is_valid(_world):
    raise RuntimeError('VisCut: no valid editor world — open a level and rerun')
LEVEL = _world.get_name()
actors = eas.get_all_level_actors()

# ---------- 1. seed tags ----------
if SEED:
    seeded = 0
    with unreal.ScopedEditorTransaction('VisCut seed tags'):
        for a in actors:
            comps = a.get_components_by_class(unreal.StaticMeshComponent)
            sm = comps[0].get_editor_property('static_mesh') if comps else None
            if not sm: continue
            n = sm.get_name()
            want = False
            if n.lower().startswith('sm_stair'):
                want = True
            elif n.startswith('SM_Elevator_Wall'):
                o, e = a.get_actor_bounds(False)
                want = e.x >= SHELL_MIN_EXTENT and e.y >= SHELL_MIN_EXTENT
            if not want: continue
            tags = list(a.get_editor_property('tags'))
            if unreal.Name(TAG) not in tags:
                a.modify()
                tags.append(unreal.Name(TAG))
                a.set_editor_property('tags', tags)
                seeded += 1
    print('SEEDED %d actors with tag %s' % (seeded, TAG))

# ---------- 2. collect tagged boxes, union-find merge ----------
boxes = []
for a in actors:
    if unreal.Name(TAG) not in list(a.get_editor_property('tags')): continue
    o, e = a.get_actor_bounds(False)
    boxes.append([o.x-e.x, o.x+e.x, o.y-e.y, o.y+e.y, o.z-e.z, o.z+e.z, a.get_actor_label()])
print('TAGGED ACTORS: %d' % len(boxes))

parent = list(range(len(boxes)))
def find(i):
    while parent[i] != i: parent[i] = parent[parent[i]]; i = parent[i]
    return i
def touch(a, b):
    return (a[0] < b[1]+MERGE_GAP and a[1] > b[0]-MERGE_GAP and
            a[2] < b[3]+MERGE_GAP and a[3] > b[2]-MERGE_GAP and
            a[4] < b[5]+MERGE_GAP and a[5] > b[4]-MERGE_GAP)
for i in range(len(boxes)):
    for j in range(i+1, len(boxes)):
        if touch(boxes[i], boxes[j]):
            parent[find(i)] = find(j)
groups = {}
for i in range(len(boxes)):
    groups.setdefault(find(i), []).append(boxes[i])

cuts = []
for g in groups.values():
    x0 = min(b[0] for b in g); x1 = max(b[1] for b in g)
    y0 = min(b[2] for b in g); y1 = max(b[3] for b in g)
    z0 = min(b[4] for b in g); z1 = max(b[5] for b in g)
    cuts.append(((x0+INSET_XY, y0+INSET_XY, z0+INSET_Z_BOTTOM),
                 (x1-INSET_XY, y1-INSET_XY, z1+EXPAND_Z_TOP), g[0][6], len(g)))
cuts.sort(key=lambda c: -(c[1][0]-c[0][0]) * (c[1][1]-c[0][1]))
if len(cuts) > MAX_BOXES:
    print('WARNING: %d cut volumes but only %d box slots — dropping smallest: %s'
          % (len(cuts), MAX_BOXES, [c[2] for c in cuts[MAX_BOXES:]]))
    cuts = cuts[:MAX_BOXES]
for c in cuts:
    print('CUT BOX (%d actors, e.g. %s): (%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)'
          % (c[3], c[2], c[0][0], c[0][1], c[0][2], c[1][0], c[1][1], c[1][2]))

# ---------- 3. masked master per source root material ----------
HLSL = ['float m = 0.0;']
for i in range(1, MAX_BOXES + 1):
    HLSL.append('m = max(m, (WP.x > B%dMin.x && WP.x < B%dMax.x && WP.y > B%dMin.y && WP.y < B%dMax.y && WP.z > B%dMin.z && WP.z < B%dMax.z) ? 1.0 : 0.0);'
                % (i, i, i, i, i, i))
HLSL.append('return 1.0 - m;')
HLSL = '\n'.join(HLSL)

def ensure_masked_master(root):
    dst = '%s/%s_VisCutMask' % (MAT_DIR, root.get_name())
    if eal.does_asset_exist(dst):
        return unreal.load_asset(dst)
    m = eal.duplicate_asset(root.get_path_name().split('.')[0], dst)
    if not m: raise RuntimeError('duplicate master failed: %s' % dst)
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1600, 900)
    cust = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -1000, 1100)
    cust.set_editor_property('code', HLSL)
    cust.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    cust.set_editor_property('description', 'VisCutBoxClip')
    ins = [unreal.CustomInput()]
    ins[0].set_editor_property('input_name', 'WP')
    conns = []
    py = 700
    for i in range(1, MAX_BOXES + 1):
        for suf in ('Min', 'Max'):
            e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1600, py)
            e.set_editor_property('parameter_name', 'CutBox%d%s' % (i, suf))
            e.set_editor_property('default_value', unreal.LinearColor(0, 0, 0, 1))
            ci = unreal.CustomInput()
            ci.set_editor_property('input_name', 'B%d%s' % (i, suf))
            ins.append(ci)
            conns.append((e, 'B%d%s' % (i, suf)))
            py += 200
    cust.set_editor_property('inputs', ins)
    ok = [mel.connect_material_expressions(wp, '', cust, 'WP')]
    for e, nm in conns:
        ok.append(mel.connect_material_expressions(e, '', cust, nm))
    ok.append(mel.connect_material_property(cust, '', unreal.MaterialProperty.MP_OPACITY_MASK))
    if not all(ok): raise RuntimeError('connections failed on %s: %s' % (dst, ok))
    mel.recompile_material(m)
    eal.save_asset(dst, False)
    print('CREATED MASTER %s' % dst)
    return m

def ensure_level_mi(src_mat):
    # src_mat = material currently on the slab component
    name = src_mat.get_name()
    if name.startswith('MI_VisCut_'):
        return src_mat  # rerun — just update params
    if '_ShaftMask' in name:  # migrate off the session-38 one-shaft MI
        src_mat = unreal.load_asset('/Game/TheSignal/Materials/MI_Concrete_5_TwoSided')
        name = src_mat.get_name()
    if isinstance(src_mat, unreal.MaterialInstanceConstant):
        root = src_mat.get_base_material()
    else:
        root = src_mat
    master = ensure_masked_master(root)
    dst = '%s/MI_VisCut_%s_%s' % (MAT_DIR, LEVEL, root.get_name())
    if eal.does_asset_exist(dst):
        return unreal.load_asset(dst)
    if not isinstance(src_mat, unreal.MaterialInstanceConstant):
        raise RuntimeError('slab material %s is not a MaterialInstanceConstant — extend the rule for this case' % name)
    mi = eal.duplicate_asset(src_mat.get_path_name().split('.')[0], dst)
    mel.set_material_instance_parent(mi, master)
    mel.update_material_instance(mi)  # without this the MIC renders the parent look
    print('CREATED MI %s' % dst)
    return mi

# ---------- 4. assign + write params (no-op safe: skips when nothing changed) ----------
def desired_params():
    out = {}
    for i in range(1, MAX_BOXES + 1):
        if i <= len(cuts):
            mn, mx = cuts[i-1][0], cuts[i-1][1]
        else:
            mn = mx = (0.0, 0.0, 0.0)
        out['CutBox%dMin' % i] = mn
        out['CutBox%dMax' % i] = mx
    return out

work = []
want = desired_params()
for a in actors:
    if not any(a.get_actor_label().startswith(p) for p in TARGET_LABEL_PREFIXES): continue
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        cur = c.get_material(0)
        if not cur: continue
        mi = ensure_level_mi(cur)
        dirty_params = False
        for nm, v in want.items():
            got = mel.get_material_instance_vector_parameter_value(mi, nm)
            if abs(got.r - v[0]) > 0.5 or abs(got.g - v[1]) > 0.5 or abs(got.b - v[2]) > 0.5:
                dirty_params = True
                break
        if dirty_params or cur is not mi:
            work.append((a.get_actor_label(), c, mi, dirty_params, cur is not mi))

if not work:
    print('VISCUT: up to date — nothing to change.')
else:
    touched_mis = set()
    with unreal.ScopedEditorTransaction('VisCut apply'):
        for label, c, mi, dirty_params, reassign in work:
            if dirty_params and id(mi) not in touched_mis:
                for nm, v in want.items():
                    mel.set_material_instance_vector_parameter_value(mi, nm, unreal.LinearColor(v[0], v[1], v[2], 1))
                # CRITICAL: without update_material_instance a script-edited MIC
                # silently renders its PARENT's look (the 2026-07-12 lesson).
                mel.update_material_instance(mi)
                eal.save_asset(mi.get_path_name().split('.')[0], False)
                touched_mis.add(id(mi))
            if reassign:
                c.set_material(0, mi)
            print('ASSIGNED %s -> %s%s' % (label, mi.get_name(), ' (params updated)' if dirty_params else ''))
    print('VISCUT DONE: %d cut boxes, %d slab components touched. Level NOT saved.' % (len(cuts), len(work)))
