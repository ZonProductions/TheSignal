import unreal

bp = unreal.load_asset("/Game/Blueprints/Actors/BP_CCStartActor")
cdo = unreal.get_default_object(bp.generated_class())
cdo.tags.append("CC Start")
print(f"Tags: {cdo.tags}")
unreal.EditorAssetLibrary.save_asset("/Game/Blueprints/Actors/BP_CCStartActor")
print("Saved BP_CCStartActor with CC Start tag")
