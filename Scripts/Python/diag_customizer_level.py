import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

print(f"=== Level Actors ({len(actors)}) ===")
for a in actors:
    tags = list(a.tags) if a.tags else []
    print(f"  {a.get_class().get_name()}: {a.get_name()} tags={tags}")

# Check game mode
uesub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = uesub.get_editor_world()
ws = world.get_world_settings()
gm = ws.get_editor_property("default_game_mode")
print(f"\nGameMode override: {gm}")

# Check for CC Start tagged actors specifically
cc_starts = [a for a in actors if "CC Start" in list(a.tags)]
print(f"\nActors with 'CC Start' tag: {len(cc_starts)}")
for a in cc_starts:
    print(f"  {a.get_class().get_name()}: {a.get_name()}")
