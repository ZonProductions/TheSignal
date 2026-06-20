import unreal
OUT = "/Game/TheSignal/Animations/Melee"
moved = 0
for a in unreal.EditorAssetLibrary.list_assets("/Game", recursive=False):
    if "A_MeleePipe_Block" in a:
        nm = a.split("/")[-1].split(".")[0]
        dst = OUT + "/" + nm
        unreal.EditorAssetLibrary.rename_asset(a.split(".")[0], dst)
        ld = unreal.load_asset(dst)
        sk = ld.get_editor_property("skeleton") if ld else None
        sv = unreal.EditorAssetLibrary.save_asset(dst)
        print("MOVED %s skel=%s saved=%s" % (nm, sk.get_name() if sk else "None", sv))
        moved += 1
# Also report what's already in OUT
print("--- OUT dir block clips ---")
for a in unreal.EditorAssetLibrary.list_assets(OUT, recursive=True):
    if "Block" in a:
        ld = unreal.load_asset(a.split(".")[0])
        sk = ld.get_editor_property("skeleton") if ld else None
        print("  %s skel=%s" % (a, sk.get_name() if sk else "None"))
print("moved=%d" % moved)
