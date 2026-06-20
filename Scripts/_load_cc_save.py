import unreal

# The Customizer save slot
slot = "CC_SaveGame"
so = unreal.GameplayStatics.load_game_from_slot(slot, 0)
print("LOADED", so)
if so:
    print("CLASS", so.get_class().get_name())
    # Dump editor-exposed properties
    props = []
    try:
        for p in unreal.get_type_from_class(so.get_class()) if False else []:
            pass
    except Exception:
        pass
    # Enumerate FProperties via reflection
    cls = so.get_class()
    for prop in dir(so):
        if prop.startswith("_"):
            continue
    # Just try common property names used by Character Customizer
    for name in ["saved_characters", "characters", "character_data", "saved_data",
                 "presets", "save_slots", "character_map", "data", "saved_variables",
                 "character_names", "names"]:
        try:
            v = so.get_editor_property(name)
            print("PROP", name, "=>", type(v), v if not isinstance(v, (list, dict)) else "len=%d" % len(v))
            if isinstance(v, (list,)) and len(v):
                print("   first elem:", v[0])
        except Exception as e:
            pass
