import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Find and remove the raw Actor tagged "CC Start"
actors = sub.get_all_level_actors()
for a in actors:
    if "CC Start" in a.tags and a.get_class().get_name() == "Actor":
        print(f"Removing raw Actor: {a.get_name()} (tags: {a.tags})")
        a.destroy_actor()

# Spawn BP_CCStartActor (already has "CC Start" tag in CDO)
bp_class = unreal.load_asset("/Game/Blueprints/Actors/BP_CCStartActor")
start_actor = sub.spawn_actor_from_class(
    bp_class.generated_class(),
    unreal.Vector(200, 0, 100)
)
# Set a default design name
start_actor.set_editor_property("CharacterDesignName", "MyCharacter")
print(f"BP_CCStartActor placed: {start_actor.get_name()}, tags: {start_actor.tags}")
print(f"CharacterDesignName: {start_actor.get_editor_property('CharacterDesignName')}")

# Save level
levelsub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levelsub.save_current_level()
print("Level saved")
