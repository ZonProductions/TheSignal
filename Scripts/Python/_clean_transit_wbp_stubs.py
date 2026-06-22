import unreal
EAL = unreal.EditorAssetLibrary
for p in ["/Game/Campaign/UI/WBP_TransitMenu", "/Game/Campaign/UI/WBP_TransitRow"]:
    if EAL.does_asset_exist(p):
        ok = EAL.delete_asset(p)
        print("deleted", p, ok)
    else:
        print("not present (clean)", p)
