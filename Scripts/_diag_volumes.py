"""Inspect ALL PostProcessVolumes + LightmassImportanceVolume in the demo
map: bounds, priority, every overridden setting. Read-only."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.log(f'MAP: {w.get_name()}')

for a in actor_sub.get_all_level_actors():
    cls = a.get_class().get_name()
    if cls == 'PostProcessVolume':
        unb = a.get_editor_property('unbound')
        en = a.get_editor_property('enabled')
        o, e = a.get_actor_bounds(False)
        unreal.log(f'--- PPV "{a.get_actor_label()}" enabled={en} unbound={unb} '
                   f'priority={a.get_editor_property("priority")} blend={a.get_editor_property("blend_radius")}')
        unreal.log(f'    bounds X {o.x-e.x:.0f}..{o.x+e.x:.0f} Y {o.y-e.y:.0f}..{o.y+e.y:.0f} Z {o.z-e.z:.0f}..{o.z+e.z:.0f}')
        s = a.get_editor_property('settings')
        n = 0
        for p in dir(s):
            if p.startswith('override_') and s.get_editor_property(p):
                vn = p.replace('override_', '')
                n += 1
                try:
                    unreal.log(f'    OVERRIDE {vn} = {s.get_editor_property(vn)}')
                except Exception:
                    unreal.log(f'    OVERRIDE {vn} (unreadable)')
        if n == 0:
            unreal.log('    (no overrides)')
    elif cls == 'LightmassImportanceVolume':
        o, e = a.get_actor_bounds(False)
        unreal.log(f'--- LightmassImportanceVolume "{a.get_actor_label()}" '
                   f'bounds X {o.x-e.x:.0f}..{o.x+e.x:.0f} Y {o.y-e.y:.0f}..{o.y+e.y:.0f}')

ws = w.get_world_settings()
unreal.log(f'force_no_precomputed_lighting = {ws.get_editor_property("force_no_precomputed_lighting")}')
reg = unreal.AssetRegistryHelpers.get_asset_registry()
bd = reg.get_assets_by_package_name('/Game/office_BigCompanyArchViz/Maps/Building1_3rdFloor_BuiltData')
unreal.log(f'map BuiltData asset: {"EXISTS (baked lighting present)" if bd else "none"}')
