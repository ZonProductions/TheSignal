import unreal

sm = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_HangedLamp")
print("LOADED:", sm.get_path_name())

# Sockets via editor property
socks = sm.get_editor_property("sockets")
print("SOCKET COUNT:", len(socks))
for s in socks:
    print("  SOCKET", s.get_editor_property("socket_name"),
          "loc:", s.get_editor_property("relative_location"))

# Material slots (which section is wire vs cone)
mats = sm.get_editor_property("static_materials")
for i, m in enumerate(mats):
    mi = m.get_editor_property("material_interface")
    print("  SLOT", i, "name:", m.get_editor_property("material_slot_name"),
          "mat:", mi.get_name() if mi else None)

# Geometry Script availability (the classes we'd use)
for cls in ["GeometryScript_StaticMeshFunctions", "GeometryScript_MeshModeling",
            "GeometryScript_MeshBooleans", "DynamicMesh", "GeometryScript_SceneUtils",
            "GeometryScriptDebug"]:
    print("  HAS", cls, "=", hasattr(unreal, cls))
