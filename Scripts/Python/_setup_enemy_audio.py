# Re-import the Crawler SFX as MONO (stereo can't spatialize/attenuate) and create a
# reusable SA_EnemyVoice attenuation asset (room-scale falloff).
import unreal

src = "C:/Users/Ommei/workspace/TheSignal/Sfx/CrawlerMono"
dest = "/Game/Audio/Crawler"
names = ["SFX_Crawler_Lurking", "SFX_Crawler_Alert", "SFX_Crawler_Attack", "SFX_Crawler_Attack2"]

tasks = []
for n in names:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src + "/" + n + ".wav")
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

EAL = unreal.EditorAssetLibrary
lk = unreal.load_asset(dest + "/SFX_Crawler_Lurking")
if lk:
    try:
        lk.set_editor_property("looping", True)
        lk.set_editor_property("virtualization_mode", unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    except Exception as e:
        unreal.log_warning("lurk prop skip: %s" % str(e))
    EAL.save_asset(dest + "/SFX_Crawler_Lurking")

# verify mono
for n in names:
    a = unreal.load_asset(dest + "/" + n)
    ch = a.get_editor_property("num_channels") if a else "?"
    unreal.log_warning("REIMPORT %s channels=%s" % (n, ch))

# --- SA_EnemyVoice attenuation asset (room-scale) ---
sa_path = "/Game/Audio/SA_EnemyVoice"
if not EAL.does_asset_exist(sa_path):
    sa = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "SA_EnemyVoice", "/Game/Audio", unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
else:
    sa = unreal.load_asset(sa_path)

s = sa.get_editor_property("attenuation")  # FSoundAttenuationSettings
for prop, val in [
    ("enable_attenuation", True),
    ("enable_spatialization", True),
    ("attenuation_shape", unreal.AttenuationShape.SPHERE),
    ("attenuation_shape_extents", unreal.Vector(350.0, 0.0, 0.0)),  # inner radius (full volume within)
    ("falloff_distance", 1200.0),                                    # fade to silence over this
]:
    try:
        s.set_editor_property(prop, val)
    except Exception as e:
        unreal.log_warning("attn prop %s skip: %s" % (prop, str(e)))
sa.set_editor_property("attenuation", s)
EAL.save_asset(sa_path)
unreal.log_warning("SA_ENEMYVOICE done inner=%s falloff=%s" % (
    s.get_editor_property("attenuation_shape_extents"), s.get_editor_property("falloff_distance")))
