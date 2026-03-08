import unreal

# Check if WBP_DesignNameInput is a proper WidgetBlueprint
asset = unreal.load_asset("/Game/Blueprints/Widgets/WBP_DesignNameInput")
print(f"Asset: {asset}")
print(f"Asset class: {asset.get_class().get_name() if asset else 'None'}")

# Check the generated class
if asset:
    gen = asset.generated_class()
    print(f"Generated class: {gen}")
    parent = gen.get_super_class() if gen else None
    print(f"Parent class: {parent}")
