"""Force UDS to apply its Time of Day variable in-editor (Python sets the
property but doesn't trigger UDS's refresh). Tries the internal update
event, falls back to rerunning the construction script."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = [a for a in actor_sub.get_all_level_actors()
       if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
unreal.log(f"Time of Day before apply: {uds.get_editor_property('Time of Day')}")

for fn in ('\U0001F4D8Time of Day', 'UserConstructionScript'):
    try:
        uds.call_method(fn)
        unreal.log(f'called: {fn!r}')
        break
    except Exception as e:
        unreal.log_warning(f'{fn!r} failed: {e}')
else:
    unreal.log_error('no apply path worked')

unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, 'claude_sky_check3.png')
