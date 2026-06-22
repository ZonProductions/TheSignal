import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

CUBE = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_Cube.SM_Cube")
WALLMAT = unreal.load_asset("/Game/office_BigCompanyArchViz/Materials/MI_WallWhite.MI_WallWhite")
THIN = 20.0  # standard regular-wall thickness (UU)

assert CUBE and WALLMAT, "missing cube/material"

targets = []
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs:
        continue
    sm = cs[0].get_editor_property("static_mesh")
    if sm and sm.get_name() == "SM_WindowWall":
        targets.append((a, cs[0]))

print("window walls to convert:", len(targets))
done = 0
for a, c in targets:
    # capture footprint BEFORE swapping the mesh
    org, ext = a.get_actor_bounds(False)   # world center + half-extent of the window panel
    fx, fy, fz = ext.x * 2.0, ext.y * 2.0, ext.z * 2.0  # full world size
    # thinnest horizontal axis = wall facing; clamp it to standard thickness
    if fx <= fy:
        sx, sy, sz = THIN / 100.0, fy / 100.0, fz / 100.0
    else:
        sx, sy, sz = fx / 100.0, THIN / 100.0, fz / 100.0
    # rebuild as a solid regular wall in the exact same space
    c.set_static_mesh(CUBE)
    a.set_actor_rotation(unreal.Rotator(roll=0, pitch=0, yaw=0), False)
    a.set_actor_location(org, False, False)
    a.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    for i in range(c.get_num_materials()):
        c.set_material(i, WALLMAT)
    done += 1

print("converted:", done)

# verify none remain
remaining = 0
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if cs:
        sm = cs[0].get_editor_property("static_mesh")
        if sm and sm.get_name() == "SM_WindowWall":
            remaining += 1
print("SM_WindowWall remaining:", remaining)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
saved = les.save_current_level()
print("level saved:", saved)
