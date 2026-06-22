import unreal

for mp in ["/Game/Office/OfficeMats/MI_Carpet02.MI_Carpet02",
           "/Game/Office/OfficeMats/MI_Carpet.MI_Carpet"]:
    mi = unreal.load_asset(mp)
    if not mi:
        print("MISS", mp); continue
    parent = mi.get_editor_property("parent")
    print("\n=== %s  parent=%s ===" % (mi.get_name(), parent.get_name() if parent else None))
    # scalar params
    for p in mi.get_editor_property("scalar_parameter_values"):
        print("   scalar:", p.parameter_info.name, "=", p.parameter_value)
    for p in mi.get_editor_property("vector_parameter_values"):
        v = p.parameter_value
        print("   vector:", p.parameter_info.name, "= (%.2f,%.2f,%.2f)" % (v.r, v.g, v.b))
    for p in mi.get_editor_property("texture_parameter_values"):
        t = p.parameter_value
        print("   texture:", p.parameter_info.name, "=", t.get_name() if t else None,
              ("%dx%d" % (t.blueprint_get_size_x(), t.blueprint_get_size_y())) if t else "")

# parent material expressions (look for TexCoord / tiling)
par = unreal.load_asset("/Game/Office/OfficeMats/MI_Carpet02.MI_Carpet02").get_editor_property("parent")
if par:
    print("\n=== parent material:", par.get_path_name(), "===")
    try:
        for e in par.get_editor_property("expressions"):
            cn = e.get_class().get_name()
            if "TextureCoordinate" in cn or "ScalarParameter" in cn or "Multiply" in cn or "WorldPosition" in cn:
                extra = ""
                if "TextureCoordinate" in cn:
                    extra = "Utiling=%s Vtiling=%s" % (e.get_editor_property("u_tiling"), e.get_editor_property("v_tiling"))
                if "ScalarParameter" in cn:
                    extra = "%s=%s" % (e.get_editor_property("parameter_name"), e.get_editor_property("default_value"))
                print("   expr", cn, extra)
    except Exception as ex:
        print("   (expr read:", ex, ")")

# SM_SillingTile mesh UV: does UV0 span 0..1 across the whole plate?
sm = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_SillingTile.SM_SillingTile")
e = sm.get_bounds().box_extent
print("\nSM_SillingTile local extent=(%.1f,%.1f,%.1f)  numUVchannels=%d" % (e.x, e.y, e.z, sm.get_num_uv_channels(0)))
