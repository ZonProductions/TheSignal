import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def relabel(mappath, bare_name):
    unreal.EditorLoadingAndSavingUtils.load_map(mappath)
    for a in eas.get_all_level_actors():
        if a.get_class().get_name()=="ZP_TransitPanel":
            dests = a.get_editor_property("destinations")
            if dests:
                dests[0].set_editor_property("display_name", unreal.Text(bare_name))
                a.set_editor_property("destinations", dests)
                print(mappath, "-> label:", bare_name)
    les.save_current_level()
relabel("/Game/Campaign/Building1_3rdFloor", "Empty Floor")
relabel("/Game/Campaign/Building1_Empty", "3rd Floor")
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Campaign/Building1_3rdFloor")
