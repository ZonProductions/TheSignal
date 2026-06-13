import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

cands = ["Update Sky", "Update Sky And Lighting", "Refresh All Components",
         "Update Sun Position And Sky Light", "Refresh", "Update",
         "Recapture Sky Light", "Build Sky", "Update Sky And Lighting Now",
         "Trigger Refresh", "Set Time of Day"]
for n in cands:
    try:
        uds.call_method(n)
        print("OK ->", n)
    except Exception as e:
        print("no:", n, "|", str(e)[:70])
