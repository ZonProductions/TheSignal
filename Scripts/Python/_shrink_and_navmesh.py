import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

# 1) Shrink crawlers a little (0.85) + collect bounds
n = 0
minv = unreal.Vector(1e9,1e9,1e9); maxv = unreal.Vector(-1e9,-1e9,-1e9)
for a in eas.get_all_level_actors():
    if "PatrolCreature" in a.get_name():
        a.set_actor_scale3d(unreal.Vector(0.85,0.85,0.85))
        a.modify()
        n += 1
        p = a.get_actor_location()
        minv = unreal.Vector(min(minv.x,p.x),min(minv.y,p.y),min(minv.z,p.z))
        maxv = unreal.Vector(max(maxv.x,p.x),max(maxv.y,p.y),max(maxv.z,p.z))
unreal.log_warning("Shrunk %d crawlers to 0.85. bounds min=(%.0f,%.0f,%.0f) max=(%.0f,%.0f,%.0f)" % (
    n, minv.x,minv.y,minv.z, maxv.x,maxv.y,maxv.z))

# 2) Spawn a NavMeshBoundsVolume covering the play area (generous)
cx = (minv.x+maxv.x)/2; cy=(minv.y+maxv.y)/2; cz=(minv.z+maxv.z)/2
nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(cx, cy, cz))
if nav:
    nav.set_actor_scale3d(unreal.Vector(90.0, 90.0, 8.0))  # default box ~200uu -> +/-9000 XY, +/-800 Z
    nav.set_actor_label("NavMeshBounds_Crawlers")
    unreal.log_warning("Spawned NavMeshBoundsVolume at (%.0f,%.0f,%.0f) scale 90x90x8" % (cx,cy,cz))
else:
    unreal.log_warning("FAILED to spawn NavMeshBoundsVolume")

unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
unreal.EditorLevelLibrary.save_current_level()
unreal.log_warning("RebuildNavigation issued + level saved")
