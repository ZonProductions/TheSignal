import unreal

# Create a new empty level
level_path = "/Game/Maps/MAP_CharacterDesigner"
success = unreal.EditorLevelLibrary.new_level(level_path)
print(f"New level created: {success}")

# Get the editor actor subsystem for spawning actors
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 1. Place a PlayerStart
ps = sub.spawn_actor_from_class(
    unreal.PlayerStart,
    unreal.Vector(0, 0, 100)
)
print(f"PlayerStart placed: {ps.get_name()}")

# 2. Place an empty Actor tagged "CC Start" — this tells CC_GameMode to spawn the customizer pawn
start_actor = sub.spawn_actor_from_class(
    unreal.Actor,
    unreal.Vector(200, 0, 100)
)
start_actor.tags.append("CC Start")
print(f"CC Start actor placed: {start_actor.get_name()}, tags: {start_actor.tags}")

# 3. Set the game mode override to CC_GameMode on the world settings
world = unreal.EditorLevelLibrary.get_editor_world()
world_settings = world.get_world_settings()

gm_class = unreal.load_asset("/Game/CharacterCustomizer/CharacterCustomizer_Core/Data/CC_GameMode")
if gm_class:
    gm_gen = gm_class.generated_class()
    world_settings.set_editor_property("GameModeOverride", gm_gen)
    print(f"GameMode set to: {gm_gen.get_name()}")
else:
    print("ERROR: Could not load CC_GameMode")

# 4. Add a floor plane so the character has something to stand on
floor = sub.spawn_actor_from_class(
    unreal.StaticMeshActor,
    unreal.Vector(0, 0, 0)
)
plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
if plane_mesh:
    floor_comp = floor.static_mesh_component
    floor_comp.set_static_mesh(plane_mesh)
    floor.set_actor_scale3d(unreal.Vector(10, 10, 1))
    print(f"Floor placed: {floor.get_name()}")

# 5. Add a directional light
light = sub.spawn_actor_from_class(
    unreal.DirectionalLight,
    unreal.Vector(0, 0, 500)
)
light.set_actor_rotation(unreal.Rotator(roll=0, pitch=-45, yaw=0), False)
print(f"Light placed: {light.get_name()}")

# 6. Save the level
unreal.EditorLevelLibrary.save_current_level()
print(f"Level saved: {level_path}")
print("Done! Open MAP_CharacterDesigner and hit Play to test the CC customizer.")
