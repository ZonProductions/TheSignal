"""
Create Void Ink material instances for creature variants.
All are children of M_Corruption (Procedural Monster base material).

Variants:
  MI_VoidCrawler     — Deep black ink, subtle blue-purple vein glow, wet metallic sheen
  MI_VoidOozling     — Slightly translucent dark, oily iridescent, smoother
  MI_VoidTentacle    — Matte dark, organic vein texture prominent, less shiny
  MI_VoidFlesh       — Base void template (shared reference)

All go to /Game/TheSignal/Materials/Creatures/
"""

import unreal

# Paths
PARENT_MATERIAL_PATH = "/Game/ProceduralMonster/ProceduralMonster/Materials/M_Corruption"
OUTPUT_DIR = "/Game/TheSignal/Materials/Creatures"

# Load parent material
parent_mat = unreal.load_asset(PARENT_MATERIAL_PATH)
if parent_mat is None:
    raise RuntimeError(f"Cannot load parent material: {PARENT_MATERIAL_PATH}")

print(f"Loaded parent material: {parent_mat.get_name()}")

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Ensure output directory exists
if not eal.does_directory_exist(OUTPUT_DIR):
    eal.make_directory(OUTPUT_DIR)
    print(f"Created directory: {OUTPUT_DIR}")

# ── Material Variant Definitions ──────────────────────────────────────────

VARIANTS = {
    "MI_VoidFlesh": {
        # Base void template — deep black ink
        "Color": unreal.LinearColor(r=0.02, g=0.02, b=0.04, a=1.0),
        "Metallic": 0.85,
        "Roughness": 0.08,
        "Specular": 0.3,
        "Intensity": 1.5,
        "NormalScale": 10.0,
        "TectureScale": 1.0,  # Note: typo is in original material param name
        "SteadyTextureScale": 2.0,
    },
    "MI_VoidCrawler": {
        # Crawler — wet ink sheen, subtle blue-purple vein luminance
        "Color": unreal.LinearColor(r=0.015, g=0.01, b=0.06, a=1.0),
        "Metallic": 0.92,
        "Roughness": 0.05,
        "Specular": 0.35,
        "Intensity": 1.8,
        "NormalScale": 12.0,
        "TectureScale": 0.8,
        "SteadyTextureScale": 1.5,
    },
    "MI_VoidOozling": {
        # Oozling — oily, iridescent, slightly lighter void (still very dark)
        "Color": unreal.LinearColor(r=0.03, g=0.025, b=0.05, a=1.0),
        "Metallic": 0.95,
        "Roughness": 0.03,
        "Specular": 0.5,
        "Intensity": 2.0,
        "NormalScale": 6.0,
        "TectureScale": 1.5,
        "SteadyTextureScale": 3.0,
    },
    "MI_VoidTentacle": {
        # Wall tentacle — matte dark, organic veins prominent, less shiny
        "Color": unreal.LinearColor(r=0.025, g=0.02, b=0.03, a=1.0),
        "Metallic": 0.6,
        "Roughness": 0.25,
        "Specular": 0.15,
        "Intensity": 1.3,
        "NormalScale": 14.0,
        "TectureScale": 0.6,
        "SteadyTextureScale": 1.8,
    },
}

# ── Create Material Instances ─────────────────────────────────────────────

created = []

for name, params in VARIANTS.items():
    asset_path = f"{OUTPUT_DIR}/{name}"

    # Delete existing if present (clean rebuild)
    if eal.does_asset_exist(asset_path):
        eal.delete_asset(asset_path)
        print(f"  Deleted existing: {asset_path}")

    # Create material instance
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialInstanceConstantFactoryNew()

    mi = asset_tools.create_asset(
        asset_name=name,
        package_path=OUTPUT_DIR,
        asset_class=unreal.MaterialInstanceConstant,
        factory=factory
    )

    if mi is None:
        print(f"  FAILED to create: {name}")
        continue

    # Set parent material
    mel.set_material_instance_parent(mi, parent_mat)

    # Set scalar parameters
    for param_name in ["Metallic", "Roughness", "Specular", "Intensity", "NormalScale", "TectureScale", "SteadyTextureScale"]:
        if param_name in params:
            mel.set_material_instance_scalar_parameter_value(mi, param_name, params[param_name])

    # Set vector parameter (Color)
    if "Color" in params:
        mel.set_material_instance_vector_parameter_value(mi, "Color", params["Color"])

    # Save
    eal.save_asset(asset_path)

    color = params["Color"]
    print(f"  Created {name}: Color=({color.r:.3f}, {color.g:.3f}, {color.b:.3f}), Metallic={params['Metallic']}, Roughness={params['Roughness']}")
    created.append(name)

print(f"\n=== DONE: Created {len(created)} void material instances in {OUTPUT_DIR} ===")
for name in created:
    print(f"  {OUTPUT_DIR}/{name}")
