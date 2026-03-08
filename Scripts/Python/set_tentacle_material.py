"""Set TentacleMaterial default on BP_PatrolCreature to MI_VoidCrawler."""
import unreal

bp = unreal.load_asset('/Game/TheSignal/Blueprints/Enemy/BP_PatrolCreature')
mi = unreal.load_asset('/Game/TheSignal/Materials/Creatures/MI_VoidCrawler')
print(f"MI loaded: {mi.get_name() if mi else 'None'}")

cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property('TentacleMaterial', mi)

# Verify
mat = cdo.get_editor_property('TentacleMaterial')
print(f"TentacleMaterial after set: {mat.get_name() if mat else 'None'}")

# Save BP
unreal.EditorAssetLibrary.save_asset('/Game/TheSignal/Blueprints/Enemy/BP_PatrolCreature')
print("BP_PatrolCreature saved.")
