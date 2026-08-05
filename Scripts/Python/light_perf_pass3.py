# ResearchFacility light perf pass 3 (2026-08-02) - reusable verify / re-apply / restore tool.
# Run via MCP Python endpoint:
#   curl -s -X POST http://localhost:9847/api/python -H "Content-Type: application/json" \
#     -d "{\"code\":\"exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/light_perf_pass3.py').read())\"}"
#
# MODE:
#   "verify"  (default) - check the pass is still intact (the June 2026 pass silently reverted;
#                         run this any time ResearchFacility lighting perf regresses)
#   "apply"   - re-apply the full pass (shadow drops + trims + draw distances) and save
#   "restore" - full revert to pre-pass state from light_pass3_snapshot_20260802.json and save
#
# WHAT THE PASS IS (see checkpoint 2026-08-02_researchfacility_light_perf_pass3.md):
#   - 44 adversarially-verified cast_shadows=False (fills/sconces/dim redundants), 205->161 casters
#   - 2 geometry-gated radius trims: SpotLight7 1506->1179, BP_Light_67 1494->831
#   - max_draw_distance on every light that had none: BP-class lights 9500 HARD cull (fade range
#     does NOT persist on BP light instances - reverts on save; 9500 > 8130 longest sightline so
#     no fade needed); native PointLight/SpotLight + ZP_/misc: 6000/1500 (r<=350) or 9500/2000.
#   - dev-preset 4500/4000 lights (BP_Light_1/50/63/67 labels, BP_Light_2_C class) untouched.
import unreal, json, os

MODE = "verify"

HERE = "C:/Users/Ommei/workspace/TheSignal/Scripts/Python"
SNAPSHOT = HERE + "/light_pass3_snapshot_20260802.json"
DROPLIST = HERE + "/light_pass3_droplist_20260802.json"
TRIMS = {
    # pass 3 geometry-gated trims + pass 4 room-scale cuts (dev-ordered 2026-08-02: kill the
    # stacked blinding wing; look change accepted)
    "SpotLight7": 900.0, "BP_Light_67": 831.0, "BP_Light_63": 900.0,
    "BP_Light_1": 900.0, "BP_Light_50": 900.0,
    "BP_Light_20": 550.0, "BP_Light_21": 550.0, "BP_Light_22": 550.0, "BP_Light_23": 550.0,
    "BP_Light_27": 550.0, "BP_Light_30": 550.0, "BP_Light_36": 550.0, "BP_Light_40": 550.0,
    "BP_Light_41": 550.0, "BP_Light_42": 550.0, "BP_Light_43": 550.0, "BP_Light_44": 550.0,
    "BP_Light_45": 550.0, "BP_Light_46": 550.0, "BP_Light_47": 550.0, "BP_Light_49": 550.0,
    "BP_Light_53": 550.0,
    "SpotLight9": 800.0, "PointLight59": 800.0, "PointLight62": 800.0, "PointLight3_9": 750.0,
}
BP_CLASSES = ("BP_Light_1_C", "BP_Light_2_C", "BP_HorrorLight_C", "BP_Light_4_C")

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = ues.get_editor_world()
if world is None or ues.get_game_world() is not None:
    raise RuntimeError("no editor world / PIE running - abort")
if world.get_name() != "ResearchFacility":
    raise RuntimeError("wrong level open: %s (need ResearchFacility)" % world.get_name())

snapshot = json.load(open(SNAPSHOT))
droplist = set(json.load(open(DROPLIST))["drop"])
actors = {a.get_name(): a for a in eas.get_all_level_actors()}

def comp_of(s):
    a = actors.get(s["actor"])
    if a is None:
        return None
    for c in a.get_components_by_class(unreal.LightComponent):
        if c.get_name() == s["comp"]:
            return c
    return None

def target_mdd(s, actor_cls_name):
    # returns (mdd, fade) the pass wants for this light; None = leave alone (dev preset)
    if s["max_draw_distance"] == 4500.0:
        return None
    if actor_cls_name in BP_CLASSES:
        return (9500.0, 0.0)
    if s["attenuation_radius"] <= 350.0:
        return (6000.0, 1500.0)
    return (9500.0, 2000.0)

problems, fixed = [], 0
deleted = []
for s in snapshot:
    c = comp_of(s)
    if c is None:
        # actor no longer in the level (dev deletions are normal - e.g. BP_HorrorLight_C_6
        # removed 2026-08-02); report but don't count as pass drift
        deleted.append(s["actor"])
        continue
    a_cls = actors[s["actor"]].get_class().get_name()
    cs = bool(c.get_editor_property("cast_shadows"))
    r = float(c.get_editor_property("attenuation_radius"))
    mdd = float(c.get_editor_property("max_draw_distance"))

    if MODE == "restore":
        c.set_editor_property("cast_shadows", s["cast_shadows"])
        c.set_editor_property("attenuation_radius", s["attenuation_radius"])
        c.set_editor_property("max_draw_distance", s["max_draw_distance"])
        c.set_editor_property("max_distance_fade_range", s["max_distance_fade_range"])
        fixed += 1
        continue

    want_cs = False if s["actor"] in droplist else s["cast_shadows"]
    want_r = TRIMS.get(s["actor"], s["attenuation_radius"])
    tm = target_mdd(s, a_cls)

    if MODE == "verify":
        if cs != want_cs:
            problems.append("cast_shadows %s: %s (want %s)" % (s["actor"], cs, want_cs))
        if abs(r - want_r) > 1.0:
            problems.append("radius %s: %.0f (want %.0f)" % (s["actor"], r, want_r))
        if tm is not None and mdd != tm[0]:
            problems.append("mdd %s: %.0f (want %.0f)" % (s["actor"], mdd, tm[0]))
    elif MODE == "apply":
        c.set_editor_property("cast_shadows", want_cs)
        c.set_editor_property("attenuation_radius", want_r)
        if tm is not None:
            c.set_editor_property("max_draw_distance", tm[0])
            c.set_editor_property("max_distance_fade_range", tm[1])
        fixed += 1

if MODE in ("apply", "restore"):
    saved = les.save_current_level()
    print("%s: %d lights processed, saved=%s" % (MODE.upper(), fixed, saved))
    print("Re-run with MODE='verify' to confirm.")
else:
    casters = 0
    for s in snapshot:
        c = comp_of(s)
        if c is not None and bool(c.get_editor_property("cast_shadows")):
            casters += 1
    print("VERIFY: casters=%d (pass expected 161 before any dev light deletions; pre-pass was 205)" % casters)
    if deleted:
        print("actors deleted from level since the pass (not drift): %s" % deleted)
    if problems:
        print("PASS DEGRADED - %d problems:" % len(problems))
        for p in problems[:40]:
            print("  " + p)
        print("Run with MODE='apply' to re-apply the pass.")
    else:
        print("PASS INTACT - no drift.")
