import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

exit_door = None
interact_triggers = []
for a in actors:
    lbl = a.get_actor_label()
    if lbl == 'F3_Exit_door_pCube166':
        exit_door = a
    cn = a.get_class().get_name()
    if 'InteractDoor' in cn:
        interact_triggers.append(a)

print("=== EXIT DOOR ===")
if exit_door:
    loc = exit_door.get_actor_location()
    print(f"label={exit_door.get_actor_label()} class={exit_door.get_class().get_name()} loc=({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) yaw={exit_door.get_actor_rotation().yaw:.1f}")
    comps = exit_door.get_components_by_class(unreal.StaticMeshComponent)
    for c in comps:
        m = c.get_editor_property('static_mesh')
        print(f"   mesh={m.get_name() if m else None} mobility={c.get_editor_property('mobility')}")
else:
    print("NOT FOUND")

print(f"\n=== InteractDoor TRIGGERS: {len(interact_triggers)} ===")
for t in interact_triggers:
    loc = t.get_actor_location()
    try:
        da = t.get_editor_property('DoorActor')
    except Exception as e:
        da = None
    da_lbl = da.get_actor_label() if da else 'NONE'
    dist = ''
    if exit_door:
        dist = f" distToExit={unreal.Vector.distance(loc, exit_door.get_actor_location()):.0f}"
    print(f"  {t.get_actor_label()} loc=({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) -> DoorActor={da_lbl}{dist}")
