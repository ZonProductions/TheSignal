"""Diagnose 'night light only shows at distance from building'.
Checks: PPV bounds/blend/overrides, UDS moon mobility, map built lighting."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

# 1) PPV_GlobalDarkness: bounds + every overridden setting
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == 'PPV_GlobalDarkness':
        unb = a.get_editor_property('unbound')
        o, e = a.get_actor_bounds(False)
        unreal.log(f'PPV unbound={unb} blend_radius={a.get_editor_property("blend_radius")} '
                   f'bounds X {o.x-e.x:.0f}..{o.x+e.x:.0f} Y {o.y-e.y:.0f}..{o.y+e.y:.0f} Z {o.z-e.z:.0f}..{o.z+e.z:.0f}')
        s = a.get_editor_property('settings')
        for p in dir(s):
            if p.startswith('override_') and s.get_editor_property(p):
                val_name = p.replace('override_', '')
                try:
                    unreal.log(f'  OVERRIDE {val_name} = {s.get_editor_property(val_name)}')
                except Exception:
                    unreal.log(f'  OVERRIDE {val_name} (value unreadable)')
        break

# 2) UDS moon mobility
for a in actor_sub.get_all_level_actors():
    if 'Ultra_Dynamic_Sky' in a.get_class().get_name():
        unreal.log(f'UDS Moon Mobility = {a.get_editor_property("Moon Mobility")}')
        moon = a.get_editor_property('Moon')
        unreal.log(f'Moon component mobility = {moon.get_editor_property("mobility")}')
        break

# 3) baked lighting data on this map
import unreal as u
reg = u.AssetRegistryHelpers.get_asset_registry()
for pkg in ('/Game/office_BigCompanyArchViz/Maps/Building1_3rdFloor_BuiltData',
            '/Game/office_BigCompanyArchViz/Maps/Building1_3rdFloor'):
    found = reg.get_assets_by_package_name(pkg)
    unreal.log(f'{pkg.split(chr(47))[-1]}: {"EXISTS" if found else "none"}')
ws = w.get_world_settings()
unreal.log(f'force_no_precomputed_lighting = {ws.get_editor_property("force_no_precomputed_lighting")}')
