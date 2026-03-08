"""Fix Exit_door collision blocking doorway."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Diagnose both exit door actors
for actor in all_actors:
    name = actor.get_actor_label()
    if 'Exit_door' in name:
        loc = actor.get_actor_location()
        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            mesh = c.get_editor_property('static_mesh')
            mesh_name = mesh.get_name() if mesh else 'None'
            col_profile = c.get_collision_profile_name()
            col_enabled = c.get_collision_enabled()
            body = mesh.get_editor_property('body_setup') if mesh else None
            ct = body.get_editor_property('collision_trace_flag') if body else 'N/A'
            print(f"{name}: mesh={mesh_name}, profile={col_profile}, enabled={col_enabled}, trace={ct}, loc=({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")

# Fix: set SM_DoorExit (and any other exit door mesh) to complex-as-simple
fixed = set()
for actor in all_actors:
    name = actor.get_actor_label()
    if 'Exit_door' in name:
        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            mesh = c.get_editor_property('static_mesh')
            if mesh and mesh.get_name() not in fixed:
                body = mesh.get_editor_property('body_setup')
                if body:
                    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
                    mesh_path = mesh.get_path_name().rsplit('.', 1)[0]
                    unreal.EditorAssetLibrary.save_asset(mesh_path)
                    print(f"Fixed {mesh.get_name()} -> CTF_USE_COMPLEX_AS_SIMPLE, saved {mesh_path}")
                    fixed.add(mesh.get_name())

print(f"\nFixed {len(fixed)} mesh asset(s)")
