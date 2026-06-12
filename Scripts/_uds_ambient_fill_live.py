"""LIVE: give interiors sky ambient. UDS skylight: cast_shadows off,
intensity up. In-memory only."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]

for c in u.get_components_by_class(unreal.SkyLightComponent):
    unreal.log(f'skylight comp: visible={c.get_editor_property("visible")} '
               f'intensity={c.get_editor_property("intensity")} '
               f'cast_shadows={c.get_editor_property("cast_shadows")}')
    if c.get_editor_property('visible'):
        c.set_editor_property('cast_shadows', False)

u.set_editor_property('Sky Light Intensity', 4.0)
u.call_method('\U0001F4D8Time of Day')
for c in u.get_components_by_class(unreal.SkyLightComponent):
    if c.get_editor_property('visible'):
        unreal.log(f'AFTER: intensity={c.get_editor_property("intensity")} '
                   f'cast_shadows={c.get_editor_property("cast_shadows")}')
unreal.log('LIVE in your session — look around now')
