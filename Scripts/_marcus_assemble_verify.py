import unreal
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = [a for a in actor_sub.get_all_level_actors() if a.get_actor_label()=="PREVIEW_Marcus"]
if not acts:
    print("NO PREVIEW"); raise SystemExit
a = acts[0]

cur = a.get_editor_property("Character Load Option")
ET = type(cur)
print("enum members:", [m for m in dir(ET) if m.isupper()])
val = ET.CUSTOMIZE_IN_EDITOR
a.set_editor_property("Character Load Option", val)
print("set CLO ->", a.get_editor_property("Character Load Option"))

# Force CS rerun via transform nudge
loc = a.get_actor_location()
a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z+2.0), False, False)
a.set_actor_location(loc, False, False)

# Verify assembly: Body/Head skeletal meshes + child skeletal mesh components (apparel)
def comp_mesh(name):
    c = a.get_editor_property(name) if False else None
    return c
comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
print("SkeletalMeshComponents on actor:", len(comps))
for c in comps:
    sm = c.get_skeletal_mesh_asset() if hasattr(c,"get_skeletal_mesh_asset") else None
    print("  ", c.get_name(), "->", sm.get_name() if sm else None)
