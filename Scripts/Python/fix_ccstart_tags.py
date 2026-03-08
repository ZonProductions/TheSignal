import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

# Fix the placed BP_CCStartActor instance
for a in actors:
    if "BP_CCStartActor" in a.get_class().get_name():
        a.tags.append("CC Start")
        # Also set the default design name
        a.set_editor_property("CharacterDesignName", "MyCharacter")
        print(f"Fixed: {a.get_name()} tags={list(a.tags)} name={a.get_editor_property('CharacterDesignName')}")

# Also re-fix the CDO so future instances get the tag
bp = unreal.load_asset("/Game/Blueprints/Actors/BP_CCStartActor")
cdo = unreal.get_default_object(bp.generated_class())
if "CC Start" not in list(cdo.tags):
    cdo.tags.append("CC Start")
    print(f"CDO tags fixed: {list(cdo.tags)}")
unreal.EditorAssetLibrary.save_asset("/Game/Blueprints/Actors/BP_CCStartActor")

# Save level
levelsub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levelsub.save_current_level()
print("Level saved")
