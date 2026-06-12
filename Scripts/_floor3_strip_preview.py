"""
PREVIEW of the floor-3 strip (QuakeCon demo) — hides the would-be-deleted
set via temporarily-hidden-in-editor. NON-DESTRUCTIVE: nothing modified,
nothing dirtied. What stays VISIBLE = what the delete keeps.

Same classification the real delete will use:
  KEEP: 960 <= Z < 1480 | 'stairwell' in label | protected classes
Unhide everything again: _floor3_strip_unhide.py
"""
import unreal

KEEP_Z_MIN, KEEP_Z_MAX = 960.0, 1480.0
PROTECTED = ('PostProcessVolume', 'LightmassImportanceVolume', 'PlayerStart',
             'WorldSettings', 'Brush', 'DefaultPhysicsVolume', 'LevelBounds',
             'NavMeshBoundsVolume', 'KillZVolume', 'GroupActor')

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
keep = hide = 0
flagged = []
for a in actor_sub.get_all_level_actors():
    lbl = a.get_actor_label()
    cls = a.get_class().get_name()
    z = a.get_actor_location().z
    is_keep = (KEEP_Z_MIN <= z < KEEP_Z_MAX) or ('stairwell' in lbl.lower()) or (cls in PROTECTED)
    if is_keep:
        a.set_is_temporarily_hidden_in_editor(False)
        keep += 1
    else:
        if lbl.startswith('F3_'):
            flagged.append(f'{lbl} @Z{z:.0f}')
        a.set_is_temporarily_hidden_in_editor(True)
        hide += 1

unreal.log(f'[preview] visible (KEEP): {keep}, hidden (DELETE): {hide}')
unreal.log(f'[preview] F3-labeled actors that would be DELETED (physically off-floor): {len(flagged)}')
for f in flagged:
    unreal.log(f'  {f}')
