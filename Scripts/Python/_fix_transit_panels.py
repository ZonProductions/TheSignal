import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gc = unreal.load_asset("/Game/Campaign/UI/WBP_TransitMenu").generated_class()

def fix(mappath):
    unreal.EditorLoadingAndSavingUtils.load_map(mappath)
    panels = [a for a in eas.get_all_level_actors() if a.get_class().get_name()=="ZP_TransitPanel"]
    # dedupe: keep the first, delete extras
    for extra in panels[1:]:
        print("  removing duplicate panel:", extra.get_actor_label())
        eas.destroy_actor(extra)
    panels = [a for a in eas.get_all_level_actors() if a.get_class().get_name()=="ZP_TransitPanel"]
    for p in panels:
        p.set_editor_property("transit_menu_widget_class", gc)
        wc = p.get_editor_property("transit_menu_widget_class")
        dests = p.get_editor_property("destinations")
        print("  panel", p.get_actor_label(), "widget=", wc.get_name() if wc else None, "dests=", len(dests))
    print("MAP", ues.get_editor_world().get_name(), "panels:", len(panels), "saved:", les.save_current_level())

fix("/Game/Campaign/Building1_3rdFloor")
fix("/Game/Campaign/Building1_Empty")
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Campaign/Building1_3rdFloor")
