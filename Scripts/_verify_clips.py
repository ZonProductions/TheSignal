import unreal
paths = unreal.EditorAssetLibrary.list_assets("/Game/Marcus", recursive=True)
for p in paths:
    a = unreal.load_asset(p.split(".")[0]) if "." in p else unreal.load_asset(p)
    sk = None
    try: sk = a.get_editor_property("skeleton")
    except Exception: pass
    print(p, "| skel:", sk.get_name() if sk else "?")
