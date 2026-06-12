"""Pass 2: the blackbox is probably multiple flat pieces. Find all large
StaticMeshActors near/around the building perimeter + report window pane
materials + UDS/light state."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

BX = (-24256.0, 2045.0)
BY = (-2155.0, 1378.0)

# 1) Large meshes whose bounds overlap a band around the building (within 3000 UU
#    outside the footprint), Z reaching the floor-3 band — wall/plane candidates.
unreal.log('--- large meshes near building, Z overlapping 900..1600 ---')
count = 0
for a in actor_sub.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    o, e = a.get_actor_bounds(False)
    big = max(e.x, e.y, e.z)
    if big < 800:
        continue
    # overlaps expanded footprint?
    if (o.x + e.x < BX[0] - 3000 or o.x - e.x > BX[1] + 3000 or
        o.y + e.y < BY[0] - 3000 or o.y - e.y > BY[1] + 3000):
        continue
    if o.z + e.z < 900 or o.z - e.z > 1600:
        continue
    lbl = a.get_actor_label()
    # skip interior structure noise: report only things at/OUTSIDE the walls
    outside = (o.x - e.x <= BX[0] + 50 or o.x + e.x >= BX[1] - 50 or
               o.y - e.y <= BY[0] + 50 or o.y + e.y >= BY[1] - 50)
    if not outside:
        continue
    mats = []
    mesh = ''
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        sm = c.get_editor_property('static_mesh')
        if sm:
            mesh = sm.get_name()
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            mats.append(m.get_name() if m else 'None')
    unreal.log(f'{lbl} mesh={mesh} ext=({e.x:.0f},{e.y:.0f},{e.z:.0f}) @({o.x:.0f},{o.y:.0f},{o.z:.0f}) mats={mats}')
    count += 1
    if count > 60:
        unreal.log('... (truncated)')
        break

# 2) Window pane materials right now
unreal.log('--- window walls ---')
for a in actor_sub.get_all_level_actors():
    if 'SM_WindowWall' in a.get_actor_label():
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            mats = [c.get_material(i).get_name() if c.get_material(i) else 'None'
                    for i in range(c.get_num_materials())]
        unreal.log(f'{a.get_actor_label()} mats={mats}')

# 3) UDS light state quick check
for a in actor_sub.get_all_level_actors():
    if 'Ultra_Dynamic_Sky' in a.get_class().get_name():
        unreal.log(f"UDS: TimeOfDay={a.get_editor_property('Time of Day')} "
                   f"MoonIntensity={a.get_editor_property('Moon Light Intensity')} "
                   f"InteriorAdj={a.get_editor_property('Apply Interior Adjustments')}")
        break
