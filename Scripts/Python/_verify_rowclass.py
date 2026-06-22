import unreal
menu = unreal.load_asset("/Game/Campaign/UI/WBP_TransitMenu")
gc = menu.generated_class()
cdo = unreal.get_default_object(gc)
print("RowWidgetClass =", cdo.get_editor_property("row_widget_class"))
