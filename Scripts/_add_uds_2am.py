"""
Add Ultra Dynamic Sky to Building1_3rdFloor, night @ 2 AM (QuakeCon demo).
UDS owns sun/moon/stars/atmosphere/fog — old NightSkyAtmosphere + NightFog
are DISABLED (hidden), not deleted, so reverting is trivial.
UDS time format: military float, 0200 = 2 AM -> 200.0
"""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert '3rdFloor' in w.get_name(), f'wrong map: {w.get_name()}'

# already placed? don't double-spawn
existing = [a for a in actor_sub.get_all_level_actors()
            if 'Ultra_Dynamic_Sky' in a.get_class().get_name()]
if existing:
    uds = existing[0]
    unreal.log(f'UDS already in level: {uds.get_actor_label()}')
else:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(
        '/Game/UltraDynamicSky/Blueprints/Ultra_Dynamic_Sky')
    assert cls, 'could not load Ultra_Dynamic_Sky class'
    uds = actor_sub.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    assert uds, 'spawn failed'
    uds.set_actor_label('Ultra_Dynamic_Sky')
    unreal.log('UDS spawned')

set_ok = False
for prop in ('time_of_day', 'TimeOfDay'):
    try:
        uds.set_editor_property(prop, 200.0)
        unreal.log(f'time of day -> 0200 (via {prop})')
        set_ok = True
        break
    except Exception:
        continue
if not set_ok:
    names = [str(p.get_name()) for p in uds.get_class().properties() if 'time' in str(p.get_name()).lower()] if hasattr(uds.get_class(), 'properties') else []
    unreal.log_error(f'could not set time of day; time-ish properties: {names}')

uds.modify()

# disable the old hand-placed night sky/fog (kept for easy revert)
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() in ('NightSkyAtmosphere', 'NightFog'):
        a.modify()
        a.set_actor_hidden_in_game(True)
        a.set_is_temporarily_hidden_in_editor(True)
        unreal.log(f'disabled: {a.get_actor_label()}')

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'level saved: {ok}')
