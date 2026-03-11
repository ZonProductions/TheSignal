"""Set SM_ClosetMetal box collision to absolute padded values.
Original: x=51, y=60, z=186. Adding +60/+60/+20 padding."""
import unreal

mesh_path = '/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_ClosetMetal'
mesh = unreal.load_asset(mesh_path)
body = mesh.get_editor_property('body_setup')
agg = body.get_editor_property('agg_geom')
boxes = agg.get_editor_property('box_elems')

b = boxes[0]
b.set_editor_property('x', 111.0)
b.set_editor_property('y', 120.0)
b.set_editor_property('z', 206.0)

# Must re-set the array on agg, and agg on body
agg.set_editor_property('box_elems', boxes)
body.set_editor_property('agg_geom', agg)

unreal.EditorAssetLibrary.save_asset(mesh_path)

# Reload and verify
mesh2 = unreal.load_asset(mesh_path)
body2 = mesh2.get_editor_property('body_setup')
agg2 = body2.get_editor_property('agg_geom')
boxes2 = agg2.get_editor_property('box_elems')
b2 = boxes2[0]
print(f"Verified: x={b2.get_editor_property('x')}, y={b2.get_editor_property('y')}, z={b2.get_editor_property('z')}")
print("Saved")
