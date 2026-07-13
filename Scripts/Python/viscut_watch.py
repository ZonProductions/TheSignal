# viscut_watch.py — editor watcher that makes the VisCut rule FOLLOW its actors.
#
# Registers a throttled slate post-tick callback. Every CHECK_INTERVAL seconds
# it hashes the bounds of all 'VisCut'-tagged actors in the open level; when
# the hash changes (actor moved/added/removed, or a different level opened),
# it re-runs Scripts/Python/apply_viscut_rule.py, which recomputes the cut
# boxes from live bounds and updates the masked material params. The rule
# script is no-op safe, so unchanged levels are never dirtied.
#
# Registered at editor startup via DefaultEngine.ini PythonScriptPluginSettings
# StartupScripts. Safe to exec multiple times (re-registration is guarded).
import unreal

RULE = r'C:\Users\Ommei\workspace\TheSignal\Scripts\Python\apply_viscut_rule.py'
CHECK_INTERVAL = 2.0
TAG = 'VisCut'

if not hasattr(unreal, '_viscut_watch_state'):
    unreal._viscut_watch_state = {'accum': 0.0, 'hash': None, 'handle': None, 'busy': False}

def _viscut_hash(eas, world):
    items = [world.get_name()]
    for a in eas.get_all_level_actors():
        if unreal.Name(TAG) not in list(a.get_editor_property('tags')):
            continue
        o, e = a.get_actor_bounds(False)
        items.append('%s:%.0f,%.0f,%.0f,%.0f,%.0f,%.0f' % (a.get_actor_label(), o.x, o.y, o.z, e.x, e.y, e.z))
    items.sort()
    return '|'.join(items)

def _viscut_tick(dt):
    st = unreal._viscut_watch_state
    st['accum'] += dt
    if st['accum'] < CHECK_INTERVAL or st['busy']:
        return
    st['accum'] = 0.0

    # HARD GUARD (2026-07-12 editor crash): asset opens / map loads tick Slate from
    # their progress dialog, this callback fires mid-transition, and the editor world
    # context can be momentarily null — get_all_level_actors() then hard-ASSERTS in
    # C++ (TActorIterator check(CurrentWorld), EngineUtils.h:185). try/except cannot
    # catch a C++ assert: the world MUST be verified before ANY actor iteration.
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = ues.get_editor_world() if ues else None
    if not eas or not world or not unreal.SystemLibrary.is_valid(world):
        return  # editor world mid-transition — try again next interval

    st['busy'] = True
    try:
        h = _viscut_hash(eas, world)
        if h != st['hash']:
            st['hash'] = h
            unreal.log('[VisCut] tagged actors changed — reapplying rule')
            exec(open(RULE).read(), {})
    except Exception as e:
        unreal.log_warning('[VisCut] watcher skipped a pass: %s' % e)
    finally:
        st['busy'] = False

if unreal._viscut_watch_state['handle'] is None:
    unreal._viscut_watch_state['handle'] = unreal.register_slate_post_tick_callback(_viscut_tick)
    unreal.log('[VisCut] watcher registered (interval %.1fs)' % CHECK_INTERVAL)
else:
    unreal.log('[VisCut] watcher already registered')
