import unreal
imc = unreal.load_asset('/Game/Core/Input/IMC_Grace')
mappings = imc.get_editor_property('mappings')
for m in mappings:
    a = m.get_editor_property('action')
    k = m.get_editor_property('key')
    if a:
        unreal.log(str(a.get_name()) + ' -> ' + str(k.export_text()))
