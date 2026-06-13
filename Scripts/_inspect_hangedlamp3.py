import unreal

sm = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_HangedLamp")

# Socket by common names
for nm in ["Socket", "socket", "WireSocket", "None"]:
    try:
        s = sm.find_socket(nm)
        if s:
            print("FOUND SOCKET", nm, "loc:", s.get_editor_property("relative_location"))
    except Exception as e:
        pass

# Material slots
mats = sm.get_editor_property("static_materials")
for i, m in enumerate(mats):
    mi = m.get_editor_property("material_interface")
    print("SLOT", i, "name:", m.get_editor_property("material_slot_name"),
          "mat:", mi.get_name() if mi else None)

# Geometry Script availability
for cls in ["GeometryScript_StaticMeshFunctions", "GeometryScript_MeshModeling",
            "GeometryScript_MeshSelectionFunctions", "DynamicMesh",
            "GeometryScript_MeshBasicEditFunctions"]:
    print("HAS", cls, "=", hasattr(unreal, cls))
