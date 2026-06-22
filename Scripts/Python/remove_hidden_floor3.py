import unreal, json
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

LO, HI = 640.0, 1530.0   # SAME band verified in the visibility pass

def overlaps_band(a):
    org, ext = a.get_actor_bounds(False)
    if ext.x == 0 and ext.y == 0 and ext.z == 0:
        z = a.get_actor_location().z
        zmin = zmax = z
    else:
        zmin, zmax = org.z - ext.z, org.z + ext.z
    return (zmin <= HI) and (zmax >= LO)

targets = []
for a in eas.get_all_level_actors():
    try:
        if not overlaps_band(a):
            targets.append(a)
    except Exception:
        pass  # no transform -> keep

manifest = [{"label": a.get_actor_label(), "class": a.get_class().get_name(),
             "z": round(a.get_actor_location().z,1)} for a in targets]
with open(r"C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_removed_floor3_manifest.json","w") as f:
    json.dump(manifest, f, indent=1)
print("actors OUTSIDE floor-3 band -> removing:", len(targets))

deleted = failed = 0
for a in targets:
    try:
        if eas.destroy_actor(a): deleted += 1
        else: failed += 1
    except Exception:
        failed += 1
print("deleted:", deleted, " could-not-delete:", failed)
print("total actors now:", len(eas.get_all_level_actors()))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
print("level saved:", les.save_current_level())
