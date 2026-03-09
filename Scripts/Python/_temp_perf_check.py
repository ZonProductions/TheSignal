import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()

# Count by type
type_counts = {}
total = 0
for a in actors:
    cls = a.get_class().get_name()
    type_counts[cls] = type_counts.get(cls, 0) + 1
    total += 1

# Count lights
light_count = sum(v for k, v in type_counts.items() if "Light" in k)

# Count static meshes
sm_count = type_counts.get("StaticMeshActor", 0)

# Count visible (not hidden)
hidden_count = sum(1 for a in actors if a.is_hidden_ed())
visible_count = total - hidden_count

print(f"Total actors: {total}")
print(f"StaticMeshActors: {sm_count}")
print(f"Lights: {light_count}")
print(f"\nTop 15 actor types:")
for cls, count in sorted(type_counts.items(), key=lambda x: -x[1])[:15]:
    print(f"  {cls}: {count}")
