"""Bake the dev-approved night values (live-tuned in PIE 2026-06-12) onto
the EDITOR world UDS actor and save. These are the canonical demo values."""
import unreal

VALUES = [
    ('Time of Day', 200.0),
    ('Night Brightness', 1.0),
    ('Moon Light Intensity', 1.5),
    ('Moon Light Color', unreal.LinearColor(0.55, 0.7, 1.0, 1.0)),
    ('Night Sky Glow', 0.6),
    ('Stars Intensity', 2.0),
]

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = [a for a in actor_sub.get_all_level_actors()
       if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
uds.modify()
for p, v in VALUES:
    uds.set_editor_property(p, v)
uds.call_method('\U0001F4D8Time of Day')
sun = uds.get_editor_property('Sun')
moon = uds.get_editor_property('Moon')
unreal.log(f'baked; sun intensity={sun.get_editor_property("intensity")} moon intensity={moon.get_editor_property("intensity")}')
ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'saved: {ok} (False is expected if PIE still running — resave after)')
