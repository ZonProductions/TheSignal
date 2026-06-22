import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Floor 3 vertical band (anchored to F3 window wall span 694..1250, floor slab below, ceiling above)
LO, HI = 640.0, 1530.0

kept = 0
hidden = 0
kept_zbuckets = {}
for a in eas.get_all_level_actors():
    try:
        org, ext = a.get_actor_bounds(False)
        if ext.x == 0 and ext.y == 0 and ext.z == 0:
            zmin = zmax = a.get_actor_location().z
        else:
            zmin, zmax = org.z - ext.z, org.z + ext.z
        overlaps = (zmin <= HI) and (zmax >= LO)
        a.set_is_temporarily_hidden_in_editor(not overlaps)
        if overlaps:
            kept += 1
            b = int((org.z if (ext.x or ext.y or ext.z) else a.get_actor_location().z) // 100) * 100
            kept_zbuckets[b] = kept_zbuckets.get(b, 0) + 1
        else:
            hidden += 1
    except Exception as e:
        pass  # system actors with no transform; leave as-is

print("KEPT (visible, floor3 + bleed-through):", kept)
print("HIDDEN (other floors):", hidden)
print("\n=== center-Z of KEPT actors (sanity check) ===")
for b in sorted(kept_zbuckets):
    print("  z[%5d..%5d]: %d" % (b, b+100, kept_zbuckets[b]))
