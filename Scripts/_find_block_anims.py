import unreal
hits = [a for a in unreal.EditorAssetLibrary.list_assets("/Game", recursive=True) if "A_MeleePipe_Block" in a]
print("FOUND %d" % len(hits))
for a in hits:
    ld = unreal.load_asset(a.split(".")[0])
    sk = ld.get_editor_property("skeleton") if ld else None
    print("%s  skel=%s" % (a, sk.get_name() if sk else None))
