import unreal
eal = unreal.EditorAssetLibrary
imc = eal.load_asset('/Game/Core/Input/IMC_Grace')
for m in imc.get_editor_property('mappings'):
    k = m.get_editor_property('key')
    a = m.get_editor_property('action')
    action_name = a.get_name() if a else 'None'
    key_name = str(k.get_editor_property('key_name'))
    unreal.log(f'{action_name} <- {key_name}')
