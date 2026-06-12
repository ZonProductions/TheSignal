"""Kill the UDS occlusion 'box': set Occlusion Mode = Off on the UDS
occlusion settings, editor world + live PIE. Logs state before/after."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

unreal.log(f'enum options: {[str(v) for v in unreal.UDS_Occlusion_Mode]}')

def fix(world, tag):
    uds = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
           if 'Ultra_Dynamic_Sky' in a.get_class().get_name()]
    if not uds:
        unreal.log_warning(f'{tag}: no UDS')
        return
    u = uds[0]
    try:
        unreal.log(f'{tag}: Cached Inverted Global Occlusion = '
                   f'{u.get_editor_property("Cached Inverted Global Occlusion")}')
    except Exception:
        pass
    os_ = u.get_editor_property('Occlusion Settings')
    if not os_:
        unreal.log_warning(f'{tag}: no Occlusion Settings object')
        return
    mode = os_.get_editor_property('Occlusion Mode')
    unreal.log(f'{tag}: settings obj={os_.get_path_name()} mode BEFORE={mode}')
    off = list(unreal.UDS_Occlusion_Mode)[0]  # first enum entry = Off (verify in log)
    os_.set_editor_property('Occlusion Mode', off)
    u.modify()
    u.call_method('\U0001F4D8Time of Day')
    unreal.log(f'{tag}: mode AFTER={os_.get_editor_property("Occlusion Mode")}')

fix(ues.get_editor_world(), 'EDITOR')
gw = ues.get_game_world()
if gw:
    fix(gw, 'PIE (LIVE)')
else:
    unreal.log_warning('PIE not detected')
