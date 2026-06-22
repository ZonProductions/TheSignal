import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")

def first_ps_loc():
    best = None
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "PlayerStart" and not a.get_actor_label().startswith("TEST_Arrival_"):
            return a.get_actor_location()
    return unreal.Vector(0, 0, 120)

def clear_prev():
    for a in list(eas.get_all_level_actors()):
        l = a.get_actor_label()
        if l.startswith("TEST_TransitPanel") or l.startswith("TEST_Arrival_") or a.get_class().get_name() == "ZP_TransitPanel":
            eas.destroy_actor(a)

def place_panel(loc, dest_world_path, dest_arrival_tag, label):
    p = eas.spawn_actor_from_class(unreal.ZP_TransitPanel, loc, unreal.Rotator(0,0,0))
    pm = p.get_editor_property("panel_mesh")
    pm.set_static_mesh(CUBE)
    pm.set_editor_property("relative_scale3d", unreal.Vector(0.4, 0.2, 1.2))
    pm.set_editor_property("relative_location", unreal.Vector(0, 0, 60))
    world = unreal.load_asset(dest_world_path)
    d = unreal.ZP_TransitDestination()
    d.set_editor_property("destination_id", dest_arrival_tag)
    d.set_editor_property("display_name", unreal.Text(label))
    d.set_editor_property("target_level", world)
    d.set_editor_property("arrival_point_tag", dest_arrival_tag)
    p.set_editor_property("destinations", [d])
    p.set_actor_label("TEST_TransitPanel")
    return p

def place_ps(loc, tag):
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, loc, unreal.Rotator(0,0,0))
    ps.set_editor_property("player_start_tag", tag)
    ps.set_actor_label("TEST_Arrival_" + tag)
    return ps

def setup(dest_world, dest_tag, label, my_arrival_tag):
    clear_prev()
    base = first_ps_loc()
    p = place_panel(unreal.Vector(base.x + 200, base.y, base.z), dest_world, dest_tag, label)
    place_ps(unreal.Vector(base.x - 150, base.y + 150, base.z), my_arrival_tag)
    print("MAP", ues.get_editor_world().get_name(), "panel@", (round(p.get_actor_location().x), round(p.get_actor_location().y), round(p.get_actor_location().z)),
          "dests:", len(p.get_editor_property("destinations")))
    print("saved:", les.save_current_level())
