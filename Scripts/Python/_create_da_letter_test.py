"""
Create DA_Letter_Test — a PDA_Item data asset for a collectible letter.
Uses SM_Chassis (paper stack mesh) and MI_Stationery material.
Level designer fills in Name + Description in the Details panel.
"""
import unreal

eal = unreal.EditorAssetLibrary
af = unreal.AssetToolsHelpers.get_asset_tools()

# Check if PDA_Item class is available
pda_class = unreal.load_asset("/Game/InventorySystemPro/Blueprints/Items/Core/PDA_Item")
if not pda_class:
    unreal.log_error("PDA_Item not found!")
else:
    unreal.log("PDA_Item class: " + str(pda_class.get_name()))

# Create the data asset
da_path = "/Game/Core/Items/DA_Letter_Test"
if eal.does_asset_exist(da_path):
    unreal.log("DA_Letter_Test already exists, loading...")
    da = unreal.load_asset(da_path)
else:
    # Create using the PDA_Item blueprint as the factory class
    gen_class = pda_class.generated_class()
    da = af.create_asset("DA_Letter_Test", "/Game/Core/Items", gen_class, None)
    unreal.log("Created DA_Letter_Test")

if da:
    # Set properties
    da.set_editor_property("Name", unreal.Text("Dr. Chen's Lab Notes"))
    da.set_editor_property("Description", unreal.Text(
        "March 3rd, 2026\n\n"
        "The frequency modulation tests continue to yield unexpected results. "
        "Subject 7 reported hearing a voice during the 47.3 kHz sweep — calm, "
        "almost conversational. When asked what it said, she couldn't remember "
        "the words, only that she felt 'understood.'\n\n"
        "I've flagged this for Dr. Vasquez but she's been unreachable since "
        "Tuesday. Her keycard hasn't logged into Building 3 in four days.\n\n"
        "Something is wrong here. I can feel it.\n\n"
        "— Dr. Sarah Chen, Signal Research Division"
    ))

    # Static mesh — SM_Chassis (the paper stack)
    mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Chassis")
    if mesh:
        da.set_editor_property("StaticMesh", mesh)
        unreal.log("  StaticMesh = SM_Chassis")

    # Not droppable (documents stay in inventory)
    da.set_editor_property("bIsDroppable", False)

    # Stack of 1 (each letter is unique)
    da.set_editor_property("MaxStackAmount", 1)

    # Weight 0 (documents don't weigh anything gameplay-wise)
    da.set_editor_property("Weight", 0.0)

    # Slot size 1x1
    da.set_editor_property("DefaultSlotSize", unreal.Vector2D(1.0, 1.0))

    # Save
    eal.save_asset(da_path)
    unreal.log("[create_da_letter] DA_Letter_Test created and saved")
    unreal.log("  Name: " + str(da.get_editor_property("Name")))
    unreal.log("  Description length: " + str(len(str(da.get_editor_property("Description")))))
else:
    unreal.log_error("Failed to create DA_Letter_Test")
