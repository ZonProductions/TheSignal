import unreal, json
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

for path in ["/Game/Campaign/Building1_3rdFloor","/Game/Building1_3rdFloor",
             "/Game/office_BigCompanyArchViz/Maps/Maps_BigCompany"]:
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    w=ues.get_editor_world().get_name()
    out=[]
    for a in eas.get_all_level_actors():
        lbl=a.get_actor_label(); ll=lbl.lower(); cls=a.get_class().get_name()
        cs=a.get_components_by_class(unreal.StaticMeshComponent)
        sm=cs[0].get_editor_property("static_mesh") if cs else None
        mn=sm.get_name() if sm else ""
        iselev = ("elevator" in ll or "elevator" in mn.lower() or cls=="BP_ElevatorDoors_C")
        if iselev and "wall" not in mn.lower():
            r=a.get_actor_rotation(); s=a.get_actor_scale3d(); l=a.get_actor_location()
            mats=[(cs[0].get_material(i).get_path_name() if cs and cs[0].get_material(i) else None)
                  for i in range(cs[0].get_num_materials())] if cs else []
            out.append({"label":lbl,"cls":cls,"is_bp":cls.endswith("_C"),
                        "bp_path":a.get_class().get_path_name(),
                        "mesh":sm.get_path_name() if sm else None,
                        "loc":[l.x,l.y,l.z],"rot":[r.roll,r.pitch,r.yaw],"scl":[s.x,s.y,s.z],"mats":mats})
    print("MAP %s -> elevator actors: %d"%(w,len(out)))
    if out:
        json.dump(out, open(r"C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_elevator_restore.json","w"), indent=1)
        for o in out: print("   %-34s %-22s loc=%s rot=%s"%(o["label"],o["cls"],[round(v) for v in o["loc"]],[round(v) for v in o["rot"]]))
        break
