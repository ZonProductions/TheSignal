import unreal
bp_class = unreal.load_asset("/Game/HorrorLight/BP_HorrorLight").generated_class()
les = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
loc = unreal.Vector(0, 0, 100000)  # far away, temporary
actor = les.spawn_actor_from_class(bp_class, loc, unreal.Rotator(0,0,0))
print("spawned:", actor is not None)
if actor:
    # find spotlight intensity (post construction script -> should be randomized 8..16)
    for comp in actor.get_components_by_class(unreal.SpotLightComponent):
        print("runtime SpotLight intensity:", round(comp.get_editor_property("intensity"),3),
              "units:", comp.get_editor_property("intensity_units"),
              "LF:", comp.get_editor_property("light_function_material").get_name() if comp.get_editor_property("light_function_material") else None)
    for comp in actor.get_components_by_class(unreal.NiagaraComponent):
        a=comp.get_editor_property("asset")
        print("runtime Niagara:", a.get_name() if a else None, "active:", comp.is_active())
    les.destroy_actor(actor)
    print("test actor destroyed")
