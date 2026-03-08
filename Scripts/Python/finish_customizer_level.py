import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Floor plane
floor = sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
if plane_mesh:
    floor.static_mesh_component.set_static_mesh(plane_mesh)
    floor.set_actor_scale3d(unreal.Vector(10, 10, 1))
    print(f"Floor placed: {floor.get_name()}")

# Directional light
light = sub.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500))
light.set_actor_rotation(unreal.Rotator(roll=0, pitch=-45, yaw=0), False)
print(f"Light placed: {light.get_name()}")

# Save
levelsub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levelsub.save_current_level()
print("Level saved")
