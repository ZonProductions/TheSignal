"""Add a bright temporary point light near the ladder for testing visibility."""
import unreal

# Find the ladder actor to position the light nearby
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()
ladder = None
for a in actors:
    if 'Ladder' in a.get_name():
        ladder = a
        break

if ladder:
    loc = ladder.get_actor_location()
    # Offset light slightly in front and above
    light_loc = unreal.Vector(loc.x - 100, loc.y, loc.z + 200)
else:
    # Default position if no ladder found
    light_loc = unreal.Vector(0, 0, 300)

# Spawn a point light actor
light = subsys.spawn_actor_from_class(unreal.PointLight, light_loc)
light.set_actor_label("TEMP_TestLight_DELETE_ME")
light_comp = light.point_light_component
light_comp.set_intensity(50000.0)
light_comp.set_attenuation_radius(2000.0)
light_comp.set_light_color(unreal.LinearColor(r=1.0, g=0.95, b=0.9, a=1.0))

print(f"Spawned test light at {light_loc} near ladder '{ladder.get_name() if ladder else 'NONE'}'")
print("DELETE THIS LIGHT AFTER TESTING — labeled 'TEMP_TestLight_DELETE_ME'")
