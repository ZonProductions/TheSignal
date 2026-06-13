import unreal
bp = unreal.load_asset("/Game/Blueprints/UI/WBP_HUD")
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
print("WBP_HUD status:", bp.get_editor_property("status"))
