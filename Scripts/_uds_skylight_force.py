"""LIVE: force the UDS skylight component intensity directly. No UDS update
call afterward (it would recompute/reset). In-memory only."""
import unreal

INTENSITY = 50.0

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
for c in u.get_components_by_class(unreal.SkyLightComponent):
    if c.get_editor_property('visible'):
        c.set_editor_property('intensity', INTENSITY)
        c.set_editor_property('cast_shadows', False)
        unreal.log(f'skylight intensity -> {c.get_editor_property("intensity")} (LIVE)')
