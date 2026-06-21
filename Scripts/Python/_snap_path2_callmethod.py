import unreal
ues = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target = None
for a in ues.get_all_level_actors():
    if a.get_actor_label() == "ZP_ScytheerClimbPath2":
        target = a
        break
print("found:", target is not None)
try:
    target.call_method("SnapWallNormalsToGeometry")
    print("call_method OK")
    for i, n in enumerate(target.get_editor_property("PerPointWallNormals")):
        print("   [%d] (%.2f, %.2f, %.2f)" % (i, n.x, n.y, n.z))
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("SAVED")
except Exception as e:
    print("call_method FAILED:", e)
