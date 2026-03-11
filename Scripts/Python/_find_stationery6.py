import unreal
# Load the BigCompany map and find the stationery mesh
mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/Maps/Maps_BigCompany")
if mesh:
    unreal.log("Map loaded, checking sublevels...")

# Try loading all static meshes from BigCompany and check names
ar = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = ar.get_all_assets()
count = 0
for a in all_assets:
    name = str(a.asset_name)
    pkg = str(a.package_name)
    if "BigCompany" in pkg and ("StaticMesh" in str(a.asset_class_path) or "stationery" in name.lower()):
        if "stationery" in name.lower() or "paper" in name.lower() or "note" in name.lower():
            unreal.log(pkg + " -> " + name + " (" + str(a.asset_class_path) + ")")
            count += 1
unreal.log("Found " + str(count) + " matches")

# Also check: the mesh might be embedded in the map file itself
# Let's look for SM_Chassis submeshes since MI_Stationery references it
chassis = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Chassis")
if chassis:
    num_sections = chassis.get_num_sections(0)
    unreal.log("SM_Chassis sections: " + str(num_sections))
    for i in range(num_sections):
        mat = chassis.get_material(i)
        if mat:
            unreal.log("  Section " + str(i) + ": " + mat.get_name())
