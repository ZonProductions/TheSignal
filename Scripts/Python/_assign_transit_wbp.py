import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
wbp = unreal.load_asset("/Game/Campaign/UI/WBP_TransitMenu")
gc = wbp.generated_class()
print("widget class:", gc)

def setpanels():
    n = 0
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "ZP_TransitPanel":
            a.set_editor_property("transit_menu_widget_class", gc)
            n += 1
    print("MAP", ues.get_editor_world().get_name(), "panels set:", n, "saved:", les.save_current_level())

setpanels()
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Campaign/Building1_Empty")
setpanels()
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Campaign/Building1_3rdFloor")
