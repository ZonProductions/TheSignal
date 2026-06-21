import unreal
names = ["SFX_RANGE_IMPACT_PISTOL","SFX_RANGE_IMPACT_RIFLE","SFX_RANGE_IMPACT_SHOTGUN",
         "SFX_MELEE_IMPACT1","SFX_MELEE_IMPACT2","SFX_PIPE_HIT","SFX_PIPE_SURFACE_WALL_IMPACT"]
for n in names:
    p = "/Game/Audio/Weapons/" + n
    a = unreal.load_asset(p)
    ok = unreal.EditorAssetLibrary.save_asset(p, only_if_is_dirty=False)
    print("SAVED" if ok else "SAVEFAIL", type(a).__name__ if a else "None", p)
