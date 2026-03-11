import unreal

mesh = unreal.load_asset('/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Ladder')
if mesh:
    bounds = mesh.get_bounding_box()
    min_pt = bounds.min
    max_pt = bounds.max
    center = (min_pt + max_pt) / 2.0
    print(f"SM_Ladder Bounds:")
    print(f"  Min: X={min_pt.x:.1f} Y={min_pt.y:.1f} Z={min_pt.z:.1f}")
    print(f"  Max: X={max_pt.x:.1f} Y={max_pt.y:.1f} Z={max_pt.z:.1f}")
    print(f"  Center: X={center.x:.1f} Y={center.y:.1f} Z={center.z:.1f}")
    print(f"  Size: X={max_pt.x-min_pt.x:.1f} Y={max_pt.y-min_pt.y:.1f} Z={max_pt.z-min_pt.z:.1f}")
else:
    print("ERROR: Could not load SM_Ladder")
