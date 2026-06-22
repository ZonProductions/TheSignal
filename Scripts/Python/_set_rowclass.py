import unreal
menu = unreal.load_asset("/Game/Campaign/UI/WBP_TransitMenu")
row = unreal.load_asset("/Game/Campaign/UI/WBP_TransitRow")
rowclass = row.generated_class()
cdo = menu.generated_class().get_default_object()
cdo.set_editor_property("row_widget_class", rowclass)
unreal.EditorAssetLibrary.save_asset("/Game/Campaign/UI/WBP_TransitMenu")
print("RowWidgetClass now:", cdo.get_editor_property("row_widget_class"))
