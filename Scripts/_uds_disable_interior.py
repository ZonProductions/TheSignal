"""Disable UDS automatic interior adjustments (it was suppressing the night
sky/moon whenever the camera was inside the building bounds — dev saw 'UDS
doesn't start until I leave the building'). The Signal does its own interior
darkness (exposure clamp + PPV_GlobalDarkness); UDS must not meddle.
Applies to editor world + live PIE if running."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

def fix(world, tag):
    uds = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
           if 'Ultra_Dynamic_Sky' in a.get_class().get_name()]
    if not uds:
        unreal.log_warning(f'{tag}: no UDS')
        return
    u = uds[0]
    before = u.get_editor_property('Apply Interior Adjustments')
    u.modify()
    u.set_editor_property('Apply Interior Adjustments', False)
    u.call_method('\U0001F4D8Time of Day')
    unreal.log(f'{tag}: Apply Interior Adjustments {before} -> False')

fix(ues.get_editor_world(), 'editor')
gw = ues.get_game_world()
if gw:
    fix(gw, 'PIE (LIVE)')
else:
    unreal.log('PIE not running — needs a fresh Play')

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'level saved: {ok}')
