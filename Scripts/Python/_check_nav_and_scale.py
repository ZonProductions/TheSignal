import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
navvol = 0; recast = 0; crawler = None
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    if "NavMeshBoundsVolume" in cn: navvol += 1
    if "RecastNavMesh" in cn or "NavMesh" in cn and "Bounds" not in cn: recast += 1
    if "PatrolCreature" in a.get_name() and crawler is None: crawler = a
unreal.log_warning("NavMeshBoundsVolume=%d RecastNavMesh-ish=%d" % (navvol, recast))
if crawler:
    s = crawler.get_actor_scale3d()
    unreal.log_warning("Crawler %s scale=(%.2f,%.2f,%.2f)" % (crawler.get_name(), s.x, s.y, s.z))
    for c in crawler.get_components_by_class(unreal.CapsuleComponent):
        unreal.log_warning("  Capsule r=%.1f h=%.1f scale=%s" % (
            c.get_unscaled_capsule_radius(), c.get_unscaled_capsule_half_height(), c.get_relative_scale3d()))
