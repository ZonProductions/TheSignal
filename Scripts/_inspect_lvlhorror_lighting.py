import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = ar.get_assets_by_path("/Game/__ExternalActors__/Variant_Horror/Lvl_Horror",
                               recursive=True, include_only_on_disk_assets=False)
print("EXTERNAL_ACTORS:", len(assets))

# Class buckets we care about for ambient/lighting
light_classes = {"SkyLight", "DirectionalLight", "PointLight", "SpotLight",
                 "RectLight", "SkyAtmosphere", "ExponentialHeightFog"}

# Count every class so we see if UDS / a sky system exists
from collections import Counter
cc = Counter()
for ad in assets:
    cc[str(ad.asset_class_path.asset_name)] += 1
print("CLASS_COUNTS:", dict(cc))

def comp_of(actor, cls):
    for c in actor.get_components_by_class(cls):
        return c
    return None

print("=== Light/Sky actors in Lvl_Horror ===")
for ad in assets:
    cls = str(ad.asset_class_path.asset_name)
    if cls not in light_classes:
        continue
    a = ad.get_asset()
    if a is None:
        print(" ", cls, "(could not load)")
        continue
    sl = comp_of(a, unreal.SkyLightComponent)
    dl = comp_of(a, unreal.DirectionalLightComponent)
    pl = comp_of(a, unreal.PointLightComponent)
    if sl:
        print(" SKYLIGHT %s | intensity:%s mobility:%s realtime:%s" % (
            a.get_actor_label(), sl.get_editor_property("intensity"),
            sl.get_editor_property("mobility"), sl.get_editor_property("real_time_capture")))
    elif dl:
        print(" DIRECTIONAL %s | intensity:%s" % (
            a.get_actor_label(), dl.get_editor_property("intensity")))
    elif pl:
        print(" %s %s | intensity:%s" % (cls, a.get_actor_label(),
            pl.get_editor_property("intensity")))
    else:
        print(" ", cls, a.get_actor_label())
