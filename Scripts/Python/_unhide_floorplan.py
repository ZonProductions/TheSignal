import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

restored = 0
for a in eas.get_all_level_actors():
    if a.is_temporarily_hidden_in_editor():
        a.set_is_temporarily_hidden_in_editor(False)
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            c.set_visibility(True)
        restored += 1

print(f"Restored {restored} actors")
