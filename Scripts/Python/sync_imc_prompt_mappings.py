# Sync IMC mappings into the deprecated flat array the EasyGameUI prompt system reads.
# WHY (2026-08-04, "ANY glyphs" saga): EGUI's BFL_EasyInputPromptsUtilities resolves prompt keys by
# iterating InputMappingContext.Mappings — the DEPRECATED flat array. The UE 5.8 migration keeps
# real data in DefaultKeyMappings.Mappings and leaves the deprecated array EMPTY, so every
# UseInputAction prompt resolved NO key and displayed the T_Keyboard_AnyKey fallback.
# RUN THIS AFTER ANY EDIT TO THESE IMCs (new/changed mappings won't reach glyphs until re-synced):
#   curl -s -X POST http://localhost:9847/api/python -H "Content-Type: application/json" \
#     -d "{\"code\":\"exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/sync_imc_prompt_mappings.py').read())\"}"
import unreal

IMCS = [
    "/Game/EasyGameUI/Core/UINavigation/IMC_UI_Navigation",
    "/Game/Core/Input/IMC_Grace",
    "/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IMC_InventoryCharacter",
]

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if ues.get_game_world() is not None:
    print("PIE RUNNING — values will apply in memory but SAVES WILL FAIL. Re-run after PIE.")

for path in IMCS:
    imc = unreal.load_asset(path)
    if imc is None:
        print("MISSING:", path)
        continue
    inner = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
    imc.set_editor_property("Mappings", list(inner))
    saved = unreal.EditorAssetLibrary.save_loaded_asset(imc, only_if_is_dirty=False)
    print("%s: synced %d mappings -> deprecated array, saved=%s" % (imc.get_name(), len(inner), saved))
