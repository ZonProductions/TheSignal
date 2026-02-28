"""Force-save IMC_Grace with package dirty flag."""
import unreal

imc = unreal.EditorAssetLibrary.load_asset('/Game/Core/Input/IMC_Grace')
mappings = imc.get_editor_property('mappings')
count = len(mappings)
unreal.log(f'IMC_Grace has {count} mappings before save')

# Save using save_loaded_asset (bypasses dirty check)
result = unreal.EditorAssetLibrary.save_loaded_asset(imc, only_if_is_dirty=False)
unreal.log(f'save_loaded_asset result: {result}')

# Verify after save
imc2 = unreal.EditorAssetLibrary.load_asset('/Game/Core/Input/IMC_Grace')
count2 = len(imc2.get_editor_property('mappings'))
unreal.log(f'IMC_Grace has {count2} mappings after save')
