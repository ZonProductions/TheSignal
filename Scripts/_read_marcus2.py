import unreal
so = unreal.GameplayStatics.load_game_from_slot("CC_SaveGame", 0)
m = None
for nm in ["Saved Characters", "SavedCharacters", "saved_characters",
           "Saved_Characters", "savedCharacters"]:
    try:
        m = so.get_editor_property(nm)
        print("HIT prop name:", repr(nm), "->", type(m).__name__)
        break
    except Exception as e:
        print("miss", repr(nm))
if m is not None:
    keys = list(m.keys())
    print("KEYS", [str(k) for k in keys])
    target = None
    for k in keys:
        if str(k).lower() == "marcus":
            target = k; break
    if target is None and keys:
        target = keys[0]
    print("USING", str(target))
    val = m[target]
    print("VALUE TYPE", type(val).__name__)
    try:
        print("EXPORT", val.export_text()[:2000])
    except Exception as e:
        print("no export_text", e)
