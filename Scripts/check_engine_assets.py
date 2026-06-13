import unreal
for p in [
  "/Engine/EngineLightProfiles/Complex_IES",
  "/Engine/BasicShapes/Cylinder",
  "/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential",
  "/Engine/Functions/Engine_MaterialFunctions01/Density/ExponentialDensity",
]:
    print(p, "->", unreal.EditorAssetLibrary.does_asset_exist(p))
