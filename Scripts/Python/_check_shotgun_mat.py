import unreal

mesh = unreal.load_asset('/Game/Core/Meshes/SM_PoliceShotgun')
if not mesh:
    print('SM_PoliceShotgun not found!')
else:
    print(f'Mesh: {mesh.get_name()} ({mesh.get_class().get_name()})')
    mats = mesh.get_editor_property('static_materials')
    print(f'Material slots: {len(mats)}')
    for i, m in enumerate(mats):
        mat_iface = m.get_editor_property('material_interface')
        if mat_iface:
            print(f'  Slot {i}: {mat_iface.get_name()} ({mat_iface.get_path_name()})')
        else:
            print(f'  Slot {i}: None (DEFAULT GREY)')

# Also check the MI
mi = unreal.load_asset('/Game/Core/Meshes/MI_Herrington_11-87_Police')
if mi:
    print(f'\nMaterial Instance: {mi.get_name()}')
    parent = mi.get_editor_property('parent')
    print(f'  Parent: {parent.get_name() if parent else "None"}')
else:
    print('\nMI_Herrington_11-87_Police not found at /Game/Core/Meshes/')
    # Check kinemation path
    mi2 = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Weapons/Herrington_11-87/Materials/MI_Herrington_11-87_Police')
    if mi2:
        print(f'  Found at Kinemation path: {mi2.get_path_name()}')
