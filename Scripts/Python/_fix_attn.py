import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
# probe field names
txt = s.export_text()
import re
keys = re.findall(r'([A-Za-z_]+)=', txt)
unreal.log_warning("ATTN_FIELDS=%s" % str(sorted(set(k for k in keys if 'tten' in k.lower() or 'patial' in k.lower()))))
for cand in ["attenuate","b_attenuate","spatialize","b_spatialize"]:
    try:
        s.set_editor_property(cand, True)
        unreal.log_warning("SET_OK %s" % cand)
    except Exception as e:
        pass
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("ATTN_SAVED")
