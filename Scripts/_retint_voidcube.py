import unreal
MEL = unreal.MaterialEditingLibrary
mat = unreal.load_asset("/Game/HorrorLight/M_VoidCube")

# Neutral near-black glow (equal RGB so it reads black, not blue)
NEUTRAL = unreal.LinearColor(0.02, 0.02, 0.02, 1.0)

found = False
coll = mat.get_editor_property("expression_collection")
for ex in coll.expressions:
    if isinstance(ex, unreal.MaterialExpressionVectorParameter):
        if str(ex.get_editor_property("parameter_name")) == "GlowColor":
            ex.set_editor_property("default_value", NEUTRAL)
            found = True
            print("Set GlowColor ->", NEUTRAL)
print("GlowColor expression found:", found)

MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset("/Game/HorrorLight/M_VoidCube")
print("Recompiled + saved M_VoidCube. Cubes should now read black with a faint neutral glow.")
