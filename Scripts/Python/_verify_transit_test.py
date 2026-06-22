import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
print("MAP:", ues.get_editor_world().get_name())
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name(); lbl = a.get_actor_label()
    if cn == "ZP_TransitPanel":
        dests = a.get_editor_property("destinations")
        print("PANEL", lbl, "widgetClass=", a.get_editor_property("transit_menu_widget_class"))
        for d in dests:
            tl = d.get_editor_property("target_level")
            print("   dest id=%s name='%s' arrival=%s target=%s" % (
                d.get_editor_property("destination_id"),
                d.get_editor_property("display_name"),
                d.get_editor_property("arrival_point_tag"),
                tl))
    if lbl.startswith("TEST_Arrival_"):
        print("ARRIVAL PlayerStart tag=", a.get_editor_property("player_start_tag"), "loc=", (round(a.get_actor_location().x), round(a.get_actor_location().y)))
