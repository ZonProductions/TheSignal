import unreal
# Enum values
e = getattr(unreal, "Character_Load_Option", None)
print("ENUM exists:", e is not None)
if e:
    print("ENUM values:", [str(v) for v in e])
# Find the spawned preview actor
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = [a for a in actor_sub.get_all_level_actors() if a.get_actor_label() == "PREVIEW_Marcus"]
print("found preview actors:", len(acts))
if acts:
    a = acts[0]
    methods = [m for m in dir(a) if any(s in m.lower() for s in
               ["construct","rerun","mesh","customiz","local","randomi","load"])]
    print("candidate methods:", methods)
