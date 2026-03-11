import unreal
# Find all assets that reference MI_Stationery material
ar = unreal.AssetRegistryHelpers.get_asset_registry()
mi_path = "/Game/office_BigCompanyArchViz/Materials/MI_Stationery"
deps = ar.get_referencers(mi_path)
for d in deps:
    unreal.log("Referenced by: " + str(d))
