"""
THE floor-3 strip (QuakeCon demo) — Building1_3rdFloor.
Deletes everything except: floor-3 Z band, stairwell, protected classes,
night environment (sky/grass/fog, 'Night*' labels). Dev-approved via the
visibility preview (_floor3_strip_preview.py) 2026-06-12.
DOES NOT SAVE — dev confirms in viewport, then save.
"""
import unreal

KEEP_Z_MIN, KEEP_Z_MAX = 960.0, 1480.0
PROTECTED = ('PostProcessVolume', 'LightmassImportanceVolume', 'PlayerStart',
             'WorldSettings', 'Brush', 'DefaultPhysicsVolume', 'LevelBounds',
             'NavMeshBoundsVolume', 'KillZVolume',
             'SkyAtmosphere', 'SkyLight', 'DirectionalLight',
             'ExponentialHeightFog', 'VolumetricCloud', 'Landscape',
             'InstancedFoliageActor')
ENV_WORDS = ('sky', 'grass', 'landscape', 'foliage', 'cloud', 'fog',
             'atmosphere', 'terrain')

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def is_keep(a):
    lbl = a.get_actor_label()
    low = lbl.lower()
    cls = a.get_class().get_name()
    if cls == 'GroupActor':
        return True  # containers; emptied ones cleaned below
    return ((KEEP_Z_MIN <= a.get_actor_location().z < KEEP_Z_MAX)
            or 'stairwell' in low
            or cls in PROTECTED
            or lbl.startswith('Night')
            or any(s in low for s in ENV_WORDS))

kept = deleted = 0
for a in list(actor_sub.get_all_level_actors()):
    if is_keep(a):
        a.set_is_temporarily_hidden_in_editor(False)
        kept += 1
    else:
        actor_sub.destroy_actor(a)
        deleted += 1

# clean group containers whose members are gone
groups_del = 0
for a in list(actor_sub.get_all_level_actors()):
    if a.get_class().get_name() == 'GroupActor':
        try:
            members = a.get_editor_property('group_actors')
            if not [m for m in members if m]:
                actor_sub.destroy_actor(a)
                groups_del += 1
        except Exception as e:
            unreal.log_warning(f'group check failed ({e}) — leaving groups alone')
            break

remaining = len(actor_sub.get_all_level_actors())
unreal.log(f'[strip] kept {kept}, deleted {deleted}, empty groups removed {groups_del}')
unreal.log(f'[strip] actors remaining in level: {remaining}')
unreal.log('[strip] LEVEL NOT SAVED — confirm in viewport, then save')
