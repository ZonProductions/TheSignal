import unreal

bp = unreal.load_asset("/Game/Core/Gameplay/BP_Ladder")
cdo = unreal.get_default_object(bp.generated_class())
comps = cdo.get_components_by_class(unreal.StaticMeshComponent)
mesh_comp = comps[0]
ladder_mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Ladder")
mesh_comp.set_static_mesh(ladder_mesh)
unreal.EditorAssetLibrary.save_asset("/Game/Core/Gameplay/BP_Ladder")
print(f"Assigned {ladder_mesh.get_name()} to LadderMesh component")
print("BP_Ladder saved.")
