import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

# Find the card reader
card_reader = None
for a in actors:
    if "CardReader" in a.get_class().get_name():
        card_reader = a
        break

if not card_reader:
    print("No card reader found!")
else:
    cr_loc = card_reader.get_actor_location()
    print(f"Card Reader: {card_reader.get_name()} at ({cr_loc.x:.0f},{cr_loc.y:.0f},{cr_loc.z:.0f})")

# Find all actors near the card reader and check collision
print("\n=== ALL ACTORS WITHIN 500 UU OF CARD READER ===")
for a in actors:
    loc = a.get_actor_location()
    if card_reader:
        dx = cr_loc.x - loc.x
        dy = cr_loc.y - loc.y
        dz = cr_loc.z - loc.z
        dist = (dx*dx + dy*dy + dz*dz) ** 0.5
        if dist > 500 or dist < 1:
            continue
    else:
        continue

    cn = a.get_class().get_name()
    print(f"\n{a.get_name()} [{cn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) dist={dist:.0f}")

    # Check all primitive components for blocking collision
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        ct = c.get_collision_enabled()
        cp = c.get_collision_profile_name()
        loc_c = c.get_world_location() if hasattr(c, 'get_world_location') else None
        extra = ""
        if isinstance(c, unreal.BoxComponent):
            try:
                be = c.get_editor_property("box_extent")
                extra = f" extent=({be.x:.0f},{be.y:.0f},{be.z:.0f})"
            except:
                pass
        if isinstance(c, unreal.StaticMeshComponent):
            try:
                sm = c.get_editor_property("static_mesh")
                extra = f" mesh={sm.get_name() if sm else 'None'}"
            except:
                extra = " mesh=?"
        print(f"  {c.get_name()} [{type(c).__name__}] collision={ct} profile={cp}{extra}")
