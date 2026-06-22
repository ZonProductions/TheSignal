import unreal, json
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Campaign/Building1_Empty")
print("map:", ues.get_editor_world().get_name())

data=json.load(open(r"C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_elevator_restore.json"))
existing={a.get_actor_label() for a in eas.get_all_level_actors()}
made=0
for o in data:
    if o["label"] in existing:
        print("  skip (already present):", o["label"]); continue
    loc=unreal.Vector(*o["loc"]); rot=unreal.Rotator(roll=o["rot"][0],pitch=o["rot"][1],yaw=o["rot"][2])
    if o["is_bp"]:
        pkg=o["bp_path"].split(".")[0]            # /Game/.../BP_ElevatorDoors
        obj=unreal.load_asset(pkg)
    else:
        obj=unreal.load_asset(o["mesh"])
    if not obj:
        print("  FAILED load:", o["label"], o.get("bp_path") or o["mesh"]); continue
    act=eas.spawn_actor_from_object(obj, loc, rot)
    if not act:
        print("  FAILED spawn:", o["label"]); continue
    act.set_actor_scale3d(unreal.Vector(*o["scl"]))
    if not o["is_bp"] and o["mats"]:
        cs=act.get_components_by_class(unreal.StaticMeshComponent)
        if cs:
            for i,mp in enumerate(o["mats"]):
                if mp:
                    m=unreal.load_asset(mp)
                    if m: cs[0].set_material(i,m)
    act.set_actor_label(o["label"])
    made+=1
    print("  restored:", o["label"], o["cls"])

print("restored:", made)
print("level saved:", les.save_current_level())
