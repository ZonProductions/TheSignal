import unreal

found = unreal.EditorAssetLibrary.list_assets('/Game/KINEMATION', recursive=True, include_folder=False)
hits = [a for a in found if 'herrington' in a.lower() or '1187' in a.lower().replace('-', '').replace('_', '')]
unreal.log(f'--- KINEMATION Herrington assets: {len(hits)}')
for h in hits[:30]:
    unreal.log(f'  {h}')
