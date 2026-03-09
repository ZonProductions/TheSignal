"""
Create 6 Blueprint children of AZP_FloorSign, each with a different default FloorNumber.
BP_FloorSign_1 (FloorNumber=1) through BP_FloorSign_6 (FloorNumber=6).
User just drags the right one — material is set automatically on placement.
"""
import unreal

parent_class = unreal.load_class(None, "/Script/TheSignal.ZP_FloorSign")
if not parent_class:
    print("ERROR: Could not load ZP_FloorSign class")
    raise RuntimeError("ZP_FloorSign not found")

factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", parent_class)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
BP_PATH = "/Game/TheSignal/Blueprints/LevelDesign"

# Delete the unnumbered BP_FloorSign if it exists
single = f"{BP_PATH}/BP_FloorSign"
if unreal.EditorAssetLibrary.does_asset_exist(single):
    unreal.EditorAssetLibrary.delete_asset(single)
    print("Deleted unnumbered BP_FloorSign")

for i in range(1, 7):
    name = f"BP_FloorSign_{i}"
    full = f"{BP_PATH}/{name}"

    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)

    bp = asset_tools.create_asset(name, BP_PATH, unreal.Blueprint, factory)
    if bp:
        try:
            gen_class = bp.generated_class()
            cdo = gen_class.get_default_object()
            cdo.set_editor_property("floor_number", i)
        except Exception as e:
            print(f"  CDO set_editor_property failed: {e}, will try alternative")
            # Alternative: use subsystem
            try:
                subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
            except:
                pass

        unreal.EditorAssetLibrary.save_asset(full)
        print(f"Created {name} (FloorNumber={i})")
    else:
        print(f"FAILED to create {name}")
