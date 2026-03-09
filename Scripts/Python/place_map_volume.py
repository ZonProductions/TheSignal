"""
Place a MapVolume in the current level covering the full building footprint.
Run via MCP Python endpoint.
"""
import unreal

# Level bounds from geometry scan
x_min, x_max = -4761.0, 2318.0
y_min, y_max = -1750.0, 1898.0

center_x = (x_min + x_max) / 2.0
center_y = (y_min + y_max) / 2.0
center_z = 150.0  # Roughly player height — volume needs to overlap player capsule

half_x = (x_max - x_min) / 2.0 + 200.0  # Pad 200 UU each side
half_y = (y_max - y_min) / 2.0 + 200.0
half_z = 500.0  # Tall enough to cover multiple floors if needed

# Spawn AZP_MapVolume
actor_class = unreal.EditorAssetLibrary.load_blueprint_class('/Script/TheSignal.ZP_MapVolume')
if not actor_class:
    # Try loading as a native class
    actor_class = unreal.load_class(None, '/Script/TheSignal.ZP_MapVolume')

if not actor_class:
    raise RuntimeError('Could not find ZP_MapVolume class')

location = unreal.Vector(center_x, center_y, center_z)
rotation = unreal.Rotator()

volume = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
if not volume:
    raise RuntimeError('Failed to spawn MapVolume')

# Set properties
volume.set_editor_property('area_id', 'BigCompany_Main')
volume.set_editor_property('area_display_name', unreal.Text('Big Company - Main Floor'))

# Set box extent to cover the level
# The BoxComponent extent is half-size
box = volume.get_editor_property('area_bounds')
if box:
    box.set_box_extent(unreal.Vector(half_x, half_y, half_z))
    print(f'Box extent set to: {half_x:.0f} x {half_y:.0f} x {half_z:.0f}')
else:
    print('WARN: Could not find AreaBounds BoxComponent — resize manually')

volume.set_actor_label('MapVolume_BigCompany')
print(f'OK: Placed MapVolume at ({center_x:.0f}, {center_y:.0f}, {center_z:.0f})')
print(f'Area ID: BigCompany_Main')
print(f'Coverage: {x_min-200:.0f} to {x_max+200:.0f} X, {y_min-200:.0f} to {y_max+200:.0f} Y')
