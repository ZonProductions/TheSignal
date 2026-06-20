import unreal, json

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ues.get_game_world()
cls = unreal.load_object(None, "/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Customizable_Character.CC_Customizable_Character_C")
actors = unreal.GameplayStatics.get_all_actors_of_class(world, cls)

# pick the fully-assembled one (Body mesh present)
target = None
for a in actors:
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        if c.get_name() == "Body" and (c.get_skeletal_mesh_asset() if hasattr(c,"get_skeletal_mesh_asset") else None):
            target = a; break
    if target: break
print("TARGET:", target.get_actor_label() if target else None)

recipe = {"skeletalMeshes": [], "grooms": [], "materialParams": {}}

for c in target.get_components_by_class(unreal.SkeletalMeshComponent):
    sm = c.get_skeletal_mesh_asset() if hasattr(c,"get_skeletal_mesh_asset") else None
    leader = c.get_leader_pose_component() if hasattr(c,"get_leader_pose_component") else None
    entry = {"comp": c.get_name(),
             "mesh": sm.get_path_name() if sm else None,
             "leader": leader.get_name() if leader else None,
             "mats": []}
    for i, m in enumerate(c.get_materials()):
        md = {"index": i, "name": m.get_name() if m else None}
        if isinstance(m, unreal.MaterialInstanceDynamic):
            parent = m.get_base_material() if hasattr(m,"get_base_material") else None
            md["parent"] = parent.get_path_name() if parent else None
        entry["mats"].append(md)
    recipe["skeletalMeshes"].append(entry)

# Grooms
for g in target.get_components_by_class(unreal.GroomComponent):
    ga = g.get_editor_property("groom_asset")
    recipe["grooms"].append({"comp": g.get_name(),
                             "groom": ga.get_path_name() if ga else None})

# Sample key tint params off the Body skin MID
for c in target.get_components_by_class(unreal.SkeletalMeshComponent):
    if c.get_name() == "Body":
        m = c.get_material(0)
        if isinstance(m, unreal.MaterialInstanceDynamic):
            params = {}
            for pn in ["Skin Color","Skin Tint","Tint","Color","Roughness Scale"]:
                try:
                    params[pn] = str(m.get_vector_parameter_value(pn))
                except Exception:
                    pass
            recipe["materialParams"]["BodySkinMID"] = params

out = "C:/Users/Ommei/workspace/TheSignal/Scripts/marcus_recipe.json"
with open(out, "w") as f:
    json.dump(recipe, f, indent=2)
print("WROTE", out)
print(json.dumps(recipe, indent=2)[:1800])
