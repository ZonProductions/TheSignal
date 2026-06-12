"""
Survey Building1_3rdFloor before the floor-3 strip (QuakeCon demo).
Classifies by ACTUAL Z, not label prefix — dev: F5_SM_SillingTile actors at
floor-3 height are floor 3's CEILING; FloorTile_F3 is its bottom.
READ-ONLY. Deletes nothing.
"""
import unreal, collections

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()
unreal.log(f'MAP: {w.get_name()}')

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = list(actor_sub.get_all_level_actors())
unreal.log(f'total actors: {len(all_actors)}')

# 1) Z level of each floor's tile grid
tile_z = collections.defaultdict(list)
for a in all_actors:
    lbl = a.get_actor_label()
    if lbl.startswith('FloorTile_F'):
        tile_z[lbl.split('_')[1]].append(a.get_actor_location().z)
for f in sorted(tile_z):
    zs = tile_z[f]
    unreal.log(f'  {f} tiles: {len(zs)}, Z {min(zs):.0f}..{max(zs):.0f}')

# 2) SillingTile Z distribution (which are floor 3's ceiling?)
sill_z = collections.Counter()
for a in all_actors:
    if 'SillingTile' in a.get_actor_label():
        sill_z[round(a.get_actor_location().z / 50) * 50] += 1
unreal.log(f'SillingTile Z bands: {dict(sorted(sill_z.items()))}')

# 3) Everything by label prefix + Z band
bands = collections.Counter()
nonprefixed = collections.Counter()
for a in all_actors:
    lbl = a.get_actor_label()
    z = a.get_actor_location().z
    pre = lbl.split('_')[0] if '_' in lbl else lbl
    if pre in ('F1', 'F2', 'F3', 'F4', 'F5', 'FloorTile'):
        if pre == 'FloorTile':
            pre = 'FloorTile_' + lbl.split('_')[1]
        bands[f'{pre} @Z{round(z/100)*100}'] += 1
    else:
        nonprefixed[f'{a.get_class().get_name()}: {pre}'] += 1

unreal.log('--- floor-prefixed actors by label + Z band (top 40):')
for k, n in sorted(bands.items()):
    unreal.log(f'  {k}: {n}')
unreal.log('--- NON-floor-prefixed (class: label-prefix):')
for k, n in nonprefixed.most_common(40):
    unreal.log(f'  {k}: {n}')

# 4) stairwell candidates
unreal.log('--- labels containing stair/elev/well:')
seen = set()
for a in all_actors:
    lbl = a.get_actor_label()
    low = lbl.lower()
    if any(s in low for s in ('stair', 'elev', 'well')):
        key = ''.join(ch for ch in lbl if not ch.isdigit())
        if key not in seen:
            seen.add(key)
            unreal.log(f'  {lbl} @Z{a.get_actor_location().z:.0f}')
