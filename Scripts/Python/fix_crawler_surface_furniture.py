# Crawler climb-channel pass: the CrawlerSurface channel (ECC_GameTraceChannel1) defaults to
# Block, so the creature treats EVERY mesh as a climbable wall and tries to scale furniture.
# This sets any SHORT actor (full height < THRESHOLD) to IGNORE that channel, so the creature
# slides past furniture instead of climbing it. Tall walls keep blocking it (still climbable).
# Physical collision is untouched — player + creature still collide normally with furniture.
# Run per-level (like fix_all_generators2.py). Tune THRESHOLD to taste.
import unreal

THRESHOLD = 150.0  # full height (UU) below which a mesh is "furniture, don't climb"
CH = unreal.CollisionChannel.ECC_CRAWLER_SURFACE  # the climb channel (ECC_GameTraceChannel1)
IGN = unreal.CollisionResponseType.ECR_IGNORE

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

checked = 0
ignored = 0
for a in actors:
    smcs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not smcs:
        continue
    checked += 1
    origin, extent = a.get_actor_bounds(False)
    full_h = extent.z * 2.0
    if full_h < THRESHOLD:
        for c in smcs:
            try:
                c.set_collision_response_to_channel(CH, IGN)
            except Exception as e:
                unreal.log_warning("resp fail %s: %s" % (a.get_actor_label(), str(e)))
        ignored += 1

try:
    lvl = unreal.EditorLevelLibrary.get_editor_world().get_name()
    unreal.EditorLevelLibrary.save_current_level()
except Exception as e:
    lvl = "?"
    unreal.log_warning("save level skip: %s" % str(e))

unreal.log_warning("CRAWLERSURFACE_PASS level=%s mesh_actors=%d furniture(<%dUU)_ignored=%d" % (
    lvl, checked, int(THRESHOLD), ignored))
