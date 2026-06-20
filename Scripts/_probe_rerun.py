import unreal
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = [a for a in actor_sub.get_all_level_actors() if a.get_actor_label()=="PREVIEW_Marcus"]
a = acts[0]
ms = [m for m in dir(a) if any(s in m.lower() for s in ["construct","rerun","post_edit","modify","on_construction"])]
print("methods:", ms)
