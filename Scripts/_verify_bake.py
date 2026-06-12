"""Post-restart verification: load Building1_3rdFloor if needed, confirm
occlusion DA + night multiplier persisted."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
if '3rdFloor' not in w.get_name():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level('/Game/office_BigCompanyArchViz/Maps/Building1_3rdFloor')
    w = ues.get_editor_world()
unreal.log(f'map: {w.get_name()}')

u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
os_ = u.get_editor_property('Occlusion Settings')
unreal.log(f'occlusion settings = {os_.get_name()} '
           f'mode={os_.get_editor_property("Occlusion Mode")} '
           f'fractions={os_.get_editor_property("Global Occlusion Min Fraction")}/'
           f'{os_.get_editor_property("Global Occlusion Max Fraction")}')
unreal.log(f'night multiplier = {u.get_editor_property("Sky Light Color Multiplier (Night)")}')
