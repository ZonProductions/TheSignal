"""Tune UDS night feel on the LIVE PIE instance (instant feedback while the
dev stands outside). Editor-world actor is NOT touched — bake approved
values separately and save. Edit VALUES, re-run."""
import unreal

VALUES = [
    ('Night Brightness', 1.0),       # was 2.0 — flat grey ambient, back to deep
    ('Moon Light Intensity', 1.5),   # was 3.0 — still ~10x physical
    ('Moon Light Color', unreal.LinearColor(0.55, 0.7, 1.0, 1.0)),  # cool silver-blue
    ('Night Sky Glow', 0.6),         # was 1.0 — darker sky dome, stars pop
]

gw = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert gw, 'PIE not running'
uds = [a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
       if 'Ultra_Dynamic_Sky' in a.get_class().get_name()]
assert uds, 'no UDS in PIE world'
u = uds[0]
for p, v in VALUES:
    u.set_editor_property(p, v)
    unreal.log(f'{p} -> {v}')
u.call_method('\U0001F4D8Time of Day')
unreal.log('[live] applied to PIE sky')
