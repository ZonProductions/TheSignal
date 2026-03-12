"""Find all EasyGameUI widget blueprints."""
import unreal

for folder in ['/Game/EasyGameUI/', '/Game/EasyGameUI/EasyOptionsMenu/']:
    results = unreal.EditorAssetLibrary.list_assets(folder, recursive=True)
    for r in results:
        name = r.split('/')[-1].split('.')[0]
        if 'WBP' in name or 'Options' in name or 'Menu' in name:
            print(r)
