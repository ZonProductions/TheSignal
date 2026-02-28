# clear_bp_overrides.py
# Clears stale BP component template overrides on BP_GraceCharacter.
# Must be run in UE5 editor (Output Log → Python).
#
# These overrides persist from previous C++ constructor values and fight
# the current C++ defaults. After clearing, the BP inherits C++ values.

import unreal

bp_path = "/Game/Core/Player/BP_GraceCharacter"
bp = unreal.load_asset(bp_path)
if not bp:
    unreal.log_error(f"Cannot load {bp_path}")
else:
    subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsys.gather_subobject_data_for_blueprint(bp)

    cleared = []
    for h in handles:
        data = subsys.find_subobject_data_from_handle(h)
        if not data:
            continue
        name = data.get_display_name()

        # Clear FirstPersonCamera overrides (stale Z=64 from old constructor)
        if "FirstPersonCamera" in str(name):
            comp = data.get_object()
            if comp and hasattr(comp, 'set_editor_property'):
                comp.set_editor_property("relative_location", unreal.Vector(0, 0, 0))
                comp.set_editor_property("relative_rotation", unreal.Rotator(roll=0, pitch=0, yaw=0))
                cleared.append(f"FirstPersonCamera -> RelLoc=(0,0,0), RelRot=(0,0,0)")

        # Clear PlayerMesh overrides (stale Z=-152 from previous session)
        if "PlayerMesh" in str(name):
            comp = data.get_object()
            if comp and hasattr(comp, 'set_editor_property'):
                comp.set_editor_property("relative_location", unreal.Vector(0, 0, -90))
                comp.set_editor_property("relative_rotation", unreal.Rotator(roll=0, pitch=0, yaw=-90))
                cleared.append(f"PlayerMesh -> RelLoc=(0,0,-90), RelRot=(0,0,yaw=-90)")

    if cleared:
        unreal.EditorAssetLibrary.save_asset(bp_path)
        for c in cleared:
            unreal.log(f"[TheSignal] Cleared BP override: {c}")
        unreal.log(f"[TheSignal] Saved {bp_path}")
    else:
        unreal.log_warning("[TheSignal] No matching components found to clear!")
