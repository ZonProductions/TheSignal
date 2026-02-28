"""Verify IMC_Grace mappings with modifier details."""
import unreal

eal = unreal.EditorAssetLibrary
imc = eal.load_asset('/Game/Core/Input/IMC_Grace')
mappings = imc.get_editor_property('mappings')
unreal.log(f'IMC_Grace: {len(mappings)} mappings')

for i, m in enumerate(mappings):
    a = m.get_editor_property('action')
    action_name = a.get_name() if a else 'None'
    k = m.get_editor_property('key')
    key_name = str(k.get_editor_property('key_name'))
    mods = m.get_editor_property('modifiers')
    mod_count = len(mods) if mods else 0
    mod_details = []
    if mods:
        for mod in mods:
            mod_type = type(mod).__name__
            if hasattr(mod, 'get_editor_property'):
                try:
                    order = mod.get_editor_property('order')
                    mod_details.append(f'{mod_type}(order={order})')
                except:
                    mod_details.append(mod_type)
            else:
                mod_details.append(mod_type)
    unreal.log(f'  [{i}] {action_name} <- {key_name} | {mod_count} mods: {mod_details}')
