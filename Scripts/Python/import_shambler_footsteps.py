"""
Import the sliced Shambler footstep one-shots (Sfx/ShamblerFootsteps/SFX_SHAMBLER_FOOTSTEP_NN.wav)
into /Game/Audio/Shambler/Footsteps/. Idempotent: skips assets that already exist (never
overwrites — asset-safety rule). Run via MCP endpoint:
POST :9847/api/python {"code": "exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/import_shambler_footsteps.py').read())"}
"""
import os
import unreal

SRC_DIR = r"C:\Users\Ommei\workspace\TheSignal\Sfx\ShamblerFootsteps"
DEST = "/Game/Audio/Shambler/Footsteps"

tools = unreal.AssetToolsHelpers.get_asset_tools()
imported, skipped = [], []
for f in sorted(os.listdir(SRC_DIR)):
    if not f.lower().endswith(".wav"):
        continue
    name = os.path.splitext(f)[0]
    dest_path = "%s/%s" % (DEST, name)
    if unreal.EditorAssetLibrary.does_asset_exist(dest_path):
        skipped.append(name)
        continue
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC_DIR, f)
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.save = True
    task.replace_existing = False
    tools.import_asset_tasks([task])
    if unreal.EditorAssetLibrary.does_asset_exist(dest_path):
        imported.append(name)
    else:
        unreal.log_error("[footsteps] IMPORT FAILED: %s" % f)

unreal.log("[footsteps] imported=%d skipped(existing)=%d -> %s" % (len(imported), len(skipped), DEST))
