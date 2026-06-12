import unreal

found = unreal.EditorAssetLibrary.list_assets('/Game/KINEMATION/TacticalShooterPack/Animations/UE5', recursive=True, include_folder=False)
hits = [a for a in found if 'reload' in a.lower() and '/Camera/' not in a]
for h in sorted(hits):
    a = unreal.load_asset(h)
    try:
        length = a.get_play_length()
    except Exception:
        try:
            length = float(a.get_editor_property('sequence_length'))
        except Exception:
            length = -1
    unreal.log(f'{h.split("/")[-1].split(".")[0]}: {length:.2f}s')
