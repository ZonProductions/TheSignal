import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Get mesh bounds to understand geometry
for name in ['SM_ToiletKiosk', 'SM_ToiletKioskClosed']:
    mesh = unreal.load_asset(f'/Game/office_BigCompanyArchViz/StaticMesh/Probs/{name}')
    if mesh:
        bb = mesh.get_bounding_box()
        sx = bb.max.x - bb.min.x
        sy = bb.max.y - bb.min.y
        sz = bb.max.z - bb.min.z
        print(f'{name}:')
        print(f'  min=({bb.min.x:.1f}, {bb.min.y:.1f}, {bb.min.z:.1f})')
        print(f'  max=({bb.max.x:.1f}, {bb.max.y:.1f}, {bb.max.z:.1f})')
        print(f'  size=({sx:.1f} x {sy:.1f} x {sz:.1f})')

# List all kiosk actors
all_actors = subsys.get_all_level_actors()
print('\n--- Kiosk actors ---')
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    if mesh_name in ('SM_ToiletKiosk', 'SM_ToiletKioskClosed'):
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        scale = a.get_actor_scale3d()
        print(f'  {a.get_actor_label()}: {mesh_name} loc=({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) rot=(r{rot.roll:.0f},p{rot.pitch:.0f},y{rot.yaw:.0f}) scale=({scale.x:.2f},{scale.y:.2f},{scale.z:.2f})')

# Also count/list any existing cover actors
print('\n--- Existing covers ---')
for a in all_actors:
    lbl = a.get_actor_label()
    if 'KioskSignCover' in lbl or 'StallSignCover' in lbl:
        loc = a.get_actor_location()
        print(f'  {lbl}: {a.get_class().get_name()} at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})')
