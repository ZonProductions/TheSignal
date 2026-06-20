import unreal
so = unreal.GameplayStatics.load_game_from_slot("CC_SaveGame", 0)
if not so:
    print("NO SAVE")
else:
    attrs = [a for a in dir(so) if not a.startswith("__")]
    # Filter out the common UObject methods to surface BP variables
    common = set(dir(unreal.SaveGame))
    bpish = [a for a in attrs if a not in common]
    print("BP-ISH ATTRS:", bpish)
    # Try get_editor_property on each bp-ish attr that isn't a method
    for a in bpish:
        try:
            v = so.get_editor_property(a)
            kind = type(v).__name__
            if isinstance(v, (list, set)):
                print("VAR", a, "=> list len", len(v))
                if len(v):
                    print("   [0]:", v[0])
            elif isinstance(v, unreal.Map) or kind == "Map":
                try:
                    keys = list(v.keys())
                    print("VAR", a, "=> Map keys:", keys)
                except Exception as e:
                    print("VAR", a, "=> Map (keys err)", e)
            else:
                print("VAR", a, "=>", kind, str(v)[:120])
        except Exception:
            pass
