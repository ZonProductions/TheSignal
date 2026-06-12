import unreal

# 1) What weapon class does DA_AssaultRifle equip?
for da_path in ['/Game/Core/Items/DA_AssaultRifle', '/Game/Core/Items/DA_PoliceShotgun']:
    da = unreal.load_asset(da_path)
    if not da:
        unreal.log_warning(f'{da_path}: NOT FOUND')
        continue
    cls = None
    for prop in ('WeaponClass', 'EquipmentClass', 'ActorClass', 'WeaponActorClass'):
        try:
            cls = da.get_editor_property(prop)
            if cls:
                break
        except Exception:
            pass
    unreal.log(f'{da_path}: weapon class = {cls}')

# 2) SM_PoliceShotgun materials vs its source skeletal mesh
sm = unreal.load_asset('/Game/Core/Meshes/SM_PoliceShotgun')
if sm:
    mats = sm.get_editor_property('static_materials')
    unreal.log(f'SM_PoliceShotgun: {len(mats)} material slots')
    for m in mats:
        iface = m.get_editor_property('material_interface')
        unreal.log(f'  slot {m.get_editor_property("material_slot_name")}: {iface.get_path_name() if iface else "NONE"}')
else:
    unreal.log_warning('SM_PoliceShotgun not found at /Game/Core/Meshes/')

# 3) Find candidate source skeletal meshes + their materials
found = unreal.EditorAssetLibrary.list_assets('/Game', recursive=True, include_folder=False)
hits = [a for a in found if ('shotgun' in a.lower() or 'herrington' in a.lower())
        and 'BACKUP' not in a and ('SK' in a or 'Mesh' in a or 'mesh' in a)]
for h in hits[:12]:
    unreal.log(f'  candidate: {h}')
