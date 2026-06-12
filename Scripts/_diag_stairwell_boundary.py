"""Boundary forensics at the player's current PIE position:
- pawn location
- every volume (any class) whose bounds contain the pawn
- baked lighting data presence
- screenshot of current player view"""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
pawn = unreal.GameplayStatics.get_player_pawn(gw, 0)
loc = pawn.get_actor_location()
unreal.log(f'PAWN @ ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})')

for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Volume):
    o, e = a.get_actor_bounds(False)
    inside = (abs(loc.x - o.x) <= e.x and abs(loc.y - o.y) <= e.y and abs(loc.z - o.z) <= e.z)
    unb = False
    try:
        unb = a.get_editor_property('unbound')
    except Exception:
        pass
    if inside or unb:
        unreal.log(f'VOLUME {a.get_actor_label()} [{a.get_class().get_name()}] '
                   f'{"UNBOUND" if unb else "CONTAINS PAWN"} '
                   f'Z {o.z-e.z:.0f}..{o.z+e.z:.0f}')

ws = gw.get_world_settings()
unreal.log(f'force_no_precomputed_lighting = {ws.get_editor_property("force_no_precomputed_lighting")}')
reg = unreal.AssetRegistryHelpers.get_asset_registry()
bd = reg.get_assets_by_package_name('/Game/office_BigCompanyArchViz/Maps/Building1_3rdFloor_BuiltData')
unreal.log(f'BuiltData asset: {"EXISTS" if bd else "none"}')

unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, 'stairwell_boundary_here.png')
unreal.log('screenshot queued: stairwell_boundary_here.png')
