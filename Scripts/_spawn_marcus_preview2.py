import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 0. Clean up any prior preview / stray customizer NPCs we spawned
for a in actor_sub.get_all_level_actors():
    lbl = a.get_actor_label()
    if lbl in ("PREVIEW_Marcus", "CC_Customizable_NPC"):
        actor_sub.destroy_actor(a)

# 1. Marcus struct
so = unreal.GameplayStatics.load_game_from_slot("CC_SaveGame", 0)
saved = so.get_editor_property("Saved Characters")
marcus = None
for k in saved.keys():
    if str(k).lower() == "marcus":
        marcus = saved[k]; break
print("Marcus:", type(marcus).__name__)

# 2. Spawn
bp = unreal.load_asset("/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Customizable_NPC")
actor = actor_sub.spawn_actor_from_class(bp.generated_class(),
        unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
actor.set_actor_label("PREVIEW_Marcus")

# 3. Set Local Data = Marcus
actor.set_editor_property("Local Data", marcus)

# 4. Introspect assembly methods
cand = [m for m in dir(actor) if any(s in m for s in
        ["set_meshes","local_customization","set_local","customization","randomize","load_character"])]
print("assembly methods:", cand)

# 5. Call assembly: apply local data, then build meshes
def try_call(name, *args):
    fn = getattr(actor, name, None)
    if not fn:
        print("  (no method", name, ")"); return False
    try:
        r = fn(*args)
        print("  called", name, "->", r); return True
    except Exception as e:
        print("  ERR", name, e); return False

for nm in ["set_local_customization_options"]:
    try_call(nm)
for nm in ["set_meshes"]:
    try_call(nm)

actor_sub.set_selected_level_actors([actor])
print("DONE: PREVIEW_Marcus at (0,0,150)")
