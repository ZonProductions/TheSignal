import unreal

hits = [p for p in unreal.EditorAssetLibrary.list_assets("/Game", True, False)
        if ("Locker" in p or "Briefcase" in p)]
unreal.log("=== matching assets (%d) ===" % len(hits))
for p in hits:
    unreal.log("  " + p)

def dump_cdo(path):
    unreal.log("##### %s #####" % path)
    bp = unreal.load_asset(path)
    if not bp:
        unreal.log("  (load failed)")
        return
    gc = bp.generated_class() if hasattr(bp, "generated_class") else None
    cdo = unreal.get_default_object(gc) if gc else None
    if not cdo:
        unreal.log("  (no CDO)")
        return
    # components
    for c in cdo.get_components_by_class(unreal.SceneComponent):
        cn = c.get_name()
        extra = ""
        if isinstance(c, unreal.SphereComponent):
            extra = " radius=%.1f scale=%s" % (c.get_unscaled_sphere_radius(), str(c.get_relative_scale3d()))
        unreal.log("  comp: %s (%s)%s" % (cn, c.get_class().get_name(), extra))
    # bool/float props on the CDO that look interaction-relevant
    for prop in ["PreventInteractionThroughWall", "InteractionAngle", "FacingAngle",
                 "RequireFacing", "InteractionDistance", "bRequireLineOfSight",
                 "InteractionRange", "MaxInteractionAngle"]:
        try:
            v = cdo.get_editor_property(prop)
            unreal.log("  PROP %s = %s" % (prop, v))
        except Exception:
            pass

# dump the loot-locker-ish and briefcase BPs (skip data/material assets)
for p in hits:
    name = p.split("/")[-1].split(".")[0]
    if name.startswith("BP_") and ("Locker" in name or "Briefcase" in name):
        dump_cdo(p.split(".")[0])
