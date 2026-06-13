import unreal
EAL = unreal.EditorAssetLibrary
for p in ["/Game/HorrorLight/NS_DustMote", "/Game/HorrorLight/NS_VoidMote",
          "/Game/HorrorLight/NS_SporeMote", "/Game/HorrorLight/M_VoidCube",
          "/Game/HorrorLight/M_SporeCube"]:
    print(p, "->", EAL.does_asset_exist(p))
print("--- all assets in /Game/HorrorLight ---")
for a in EAL.list_assets("/Game/HorrorLight", recursive=False):
    print("  ", a)
