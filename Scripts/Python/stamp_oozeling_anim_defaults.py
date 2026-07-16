# stamp_oozeling_anim_defaults.py — ONE-TIME stamp (2026-07-13, dev request): write the Oozeling's
# anim-clip defaults onto BP_Oozeling's CDO so every clip is VISIBLE AND TUNABLE in the BP Details
# panel (the C++ lazy-load left the slots showing None in editor even though clips played at
# runtime). After this stamp the BP values win (the C++ if(!Slot) lazy fill only covers empty
# slots). Audio slots are deliberately NOT stamped — those assets don't exist yet; the empty
# EditAnywhere slots are already visible in Details, and the C++ lazy loader auto-wires the
# canonical /Game/Audio/Oozeling/ paths the session after the dev drops audio there.
#
# Run via: curl -s -X POST http://localhost:9847/api/python -H "Content-Type: application/json"
#          -d "{\"code\": \"exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/stamp_oozeling_anim_defaults.py').read())\"}"

import unreal

BP_PATH = '/Game/Enemies/Oozeling/BP_Oozeling'
SLOTS = {
    'AZP_WalkAnim':  '/Game/BigBlob/Animations/Walk_Roll/AS_BigBlob_Walk_FW.AS_BigBlob_Walk_FW',
    'AZP_RunAnim':   '/Game/BigBlob/Animations/Run_Roll/AS_BigBlob_Run_FW.AS_BigBlob_Run_FW',
    'AZP_IdleAnim':  '/Game/BigBlob/Animations/idle/AS_BigBlob_Idle_Breath.AS_BigBlob_Idle_Breath',
    'AZP_FallAnim':  '/Game/BigBlob/Animations/Jump/AS_BigBlob_Jump_Loop.AS_BigBlob_Jump_Loop',
    'AZP_EruptAnim': '/Game/BigBlob/Animations/Update1_Animations/AS_BigBlob_Hug_Attack.AS_BigBlob_Hug_Attack',
    'AZP_HitAnim':   '/Game/BigBlob/Animations/Get_hit/AS_BigBlob_GetHit.AS_BigBlob_GetHit',
    'AZP_DieAnim':   '/Game/BigBlob/Animations/Update1_Animations/AS_BigBlob_Death2.AS_BigBlob_Death2',
}

gen_class = unreal.load_class(None, BP_PATH + '.BP_Oozeling_C')
cdo = unreal.get_default_object(gen_class)
for prop, path in SLOTS.items():
    asset = unreal.load_asset(path)
    if not asset:
        print('MISSING ASSET for %s: %s' % (prop, path))
        continue
    cdo.set_editor_property(prop, asset)
    print('STAMPED %s = %s' % (prop, asset.get_name()))

bp = unreal.load_asset(BP_PATH)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
print('COMPILED')
print('SAVED: %s' % unreal.EditorAssetLibrary.save_asset(BP_PATH))

# Read-back verification
for prop in SLOTS:
    v = cdo.get_editor_property(prop)
    print('VERIFY %s -> %s' % (prop, v.get_name() if v else 'None'))
