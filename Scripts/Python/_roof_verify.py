import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LABEL = "Roof_Consolidated_Z3380"
ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"

found = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        found = a
        break

if not found:
    print("FAIL: consolidated actor not found")
else:
    print("Actor:", found.get_path_name())
    comps = found.get_components_by_class(unreal.InstancedStaticMeshComponent)
    print("ISM/HISM components:", len(comps))
    for c in comps:
        sm = c.get_editor_property('static_mesh')
        print("  comp:", c.get_name(),
              "| class:", c.get_class().get_name(),
              "| mesh:", sm.get_path_name() if sm else None,
              "| instances:", c.get_instance_count(),
              "| mobility:", str(c.get_editor_property('mobility')))
        # sample a couple of instance transforms to confirm world placement
        if c.get_instance_count() > 0:
            ok, t0 = c.get_instance_transform(0, True)
            ok2, tN = c.get_instance_transform(c.get_instance_count()-1, True)
            print("    inst[0] world loc:", t0.translation)
            print("    inst[last] world loc:", tN.translation)
