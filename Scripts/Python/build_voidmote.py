"""Void-cube effect: duplicate NS_DustMote (keep float motion) -> NS_VoidMote,
and build M_VoidCube (unlit, dark emissive 'glow') for the cube particles.
Does NOT touch the working /Game/HorrorLight/NS_DustMote. The renderer swap
(Cube mesh + this material) is done by hand in the Niagara editor afterward."""
import unreal

EAL = unreal.EditorAssetLibrary
SRC = "/Game/HorrorLight/NS_DustMote"
NS  = "/Game/HorrorLight/NS_VoidMote"
MAT = "/Game/HorrorLight/M_VoidCube"

# --- 1. Duplicate the dust system (float motion preserved) ---
if EAL.does_asset_exist(NS):
    EAL.delete_asset(NS)
dup = EAL.duplicate_asset(SRC, NS)
print("Duplicated NS_VoidMote:", dup is not None)
EAL.save_asset(NS)

# --- 2. Build the void-glow material (unlit, tunable dark emissive) ---
if EAL.does_asset_exist(MAT):
    EAL.delete_asset(MAT)
at = unreal.AssetToolsHelpers.get_asset_tools()
mat = at.create_asset("M_VoidCube", "/Game/HorrorLight", unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
MEL = unreal.MaterialEditingLibrary

color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -50)
color.set_editor_property("parameter_name", "GlowColor")
color.set_editor_property("default_value", unreal.LinearColor(0.0, 0.02, 0.06, 1.0))  # deep cold void glow
strength = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 150)
strength.set_editor_property("parameter_name", "GlowStrength")
strength.set_editor_property("default_value", 1.0)
mult = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 0)
MEL.connect_material_expressions(color, "", mult, "A")
MEL.connect_material_expressions(strength, "", mult, "B")
MEL.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
EAL.save_asset(MAT)

print("Created M_VoidCube (unlit, GlowColor + GlowStrength params).")
print("NS_VoidMote exists:", EAL.does_asset_exist(NS), "| M_VoidCube exists:", EAL.does_asset_exist(MAT))
