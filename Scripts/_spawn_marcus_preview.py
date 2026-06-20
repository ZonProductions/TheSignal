import unreal

# 1. Read Marcus's saved config struct
so = unreal.GameplayStatics.load_game_from_slot("CC_SaveGame", 0)
saved = so.get_editor_property("Saved Characters")
marcus = None
for k in saved.keys():
    if str(k).lower() == "marcus":
        marcus = saved[k]
        break
if marcus is None:
    print("MARCUS NOT FOUND"); raise SystemExit
print("Marcus struct type:", type(marcus).__name__)

# 2. Spawn CC_Customizable_NPC in the level
bp_path = "/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Customizable_NPC"
bp = unreal.load_asset(bp_path)
cls = bp.generated_class()
loc = unreal.Vector(0.0, 0.0, 200.0)
rot = unreal.Rotator(0.0, 0.0, 0.0)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor = actor_sub.spawn_actor_from_class(cls, loc, rot)
print("Spawned:", actor.get_actor_label() if actor else None)

# 3. Configure to assemble Marcus in editor
# Character Load Option: 1 = "Customize In Editor"
try:
    actor.set_editor_property("Character Load Option", 1)
except Exception as e:
    print("CLO set err:", e)
try:
    actor.set_editor_property("Local Data", marcus)
    print("Local Data set OK")
except Exception as e:
    print("LocalData set err:", e)
try:
    actor.set_editor_property("Animation In Editor", True)
except Exception as e:
    print("AnimInEditor err:", e)

# 4. Rerun construction script to assemble
actor.rerun_construction_scripts()
actor.set_actor_label("PREVIEW_Marcus")

# 5. Select + focus so the dev sees it
actor_sub.set_selected_level_actors([actor])
print("DONE. Actor label PREVIEW_Marcus at (0,0,200). Press F in viewport to focus.")
