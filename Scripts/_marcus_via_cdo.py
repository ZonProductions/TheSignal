import unreal
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Marcus struct
so = unreal.GameplayStatics.load_game_from_slot("CC_SaveGame", 0)
saved = so.get_editor_property("Saved Characters")
marcus = None
for k in saved.keys():
    if str(k).lower()=="marcus":
        marcus = saved[k]; break

bp = unreal.load_asset("/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Customizable_NPC")
cls = bp.generated_class()
cdo = unreal.get_default_object(cls)

# Save & override CDO defaults
old_clo = cdo.get_editor_property("Character Load Option")
old_ld  = cdo.get_editor_property("Local Data")
ET = type(old_clo)
cdo.set_editor_property("Character Load Option", ET.CUSTOMIZE_IN_EDITOR)
cdo.set_editor_property("Local Data", marcus)
cdo.set_editor_property("Animation In Editor", True)

# Clean old previews
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() in ("PREVIEW_Marcus","CC_Customizable_NPC"):
        actor_sub.destroy_actor(a)

# Spawn fresh -> CS runs at spawn using CDO defaults
actor = actor_sub.spawn_actor_from_class(cls, unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
actor.set_actor_label("PREVIEW_Marcus")

# Revert CDO so the pack BP stays clean
cdo.set_editor_property("Character Load Option", old_clo)
cdo.set_editor_property("Local Data", old_ld)

# Verify
comps = actor.get_components_by_class(unreal.SkeletalMeshComponent)
print("SkelMeshComponents:", len(comps))
for c in comps:
    sm = c.get_skeletal_mesh_asset() if hasattr(c,"get_skeletal_mesh_asset") else None
    print("  ", c.get_name(), "->", sm.get_name() if sm else None)
actor_sub.set_selected_level_actors([actor])
print("DONE PREVIEW_Marcus")
