import unreal
al = unreal.EditorAssetLibrary
names = ['Idle','Equip','Unequip','Attack_F','Attack_R','Attack_L','AttackHeavy_F','AttackHeavy_R']
skel = ''
for n in names:
    a = unreal.load_asset(f'/Game/A_MeleePipe_{n}.A_MeleePipe_{n}')
    if not a:
        unreal.log_error(f'{n}: LOAD FAIL')
        continue
    if n == 'Attack_F':
        skel = a.get_editor_property('skeleton').get_path_name()
    al.save_loaded_asset(a)
    ok = al.rename_asset(f'/Game/A_MeleePipe_{n}', f'/Game/TheSignal/Animations/Melee/A_MeleePipe_{n}')
    unreal.log(f'{n}: {"MOVED" if ok else "MOVE FAIL"}')
unreal.log(f'SKELETON: {skel}')
final = al.list_assets('/Game/TheSignal/Animations/Melee')
unreal.log(f'FINAL_COUNT: {len(final)}')
for f in final:
    al.save_asset(f.split('.')[0])
unreal.log('ALL SAVED')
