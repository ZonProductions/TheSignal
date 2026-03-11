"""Restore SM_ClosetMetal collision — use static mesh editor subsystem to rebuild."""
import unreal

mesh_path = '/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_ClosetMetal'
mesh = unreal.load_asset(mesh_path)
body = mesh.get_editor_property('body_setup')

# Reset to default collision flag
body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_DEFAULT)

# Use subsystem to add simple collision
subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
# Remove existing (empty) collision and add box
num_added = subsystem.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
print(f'Added {num_added} box collision(s)')

# Save
unreal.EditorAssetLibrary.save_asset(mesh_path)

# Verify
body2 = mesh.get_editor_property('body_setup')
agg = body2.get_editor_property('agg_geom')
convex = agg.get_editor_property('convex_elems')
box = agg.get_editor_property('box_elems')
print(f'Convex hulls: {len(convex)}, Box elems: {len(box)}')
print(f'Flag: {body2.get_editor_property("collision_trace_flag")}')
print('SM_ClosetMetal collision restored')
