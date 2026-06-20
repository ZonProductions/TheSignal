import unreal
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = [a for a in actor_sub.get_all_level_actors() if a.get_actor_label()=="PREVIEW_Marcus"]
if not acts:
    print("NO PREVIEW ACTOR"); raise SystemExit
a = acts[0]

# Try to set the byte-enum "Character Load Option" -> Customize In Editor (index 1)
ok = False
for val in ["NewEnumerator1", "Customize In Editor", "Customize_In_Editor"]:
    try:
        a.set_editor_property("Character Load Option", val)
        ok = True; print("set enum via string:", repr(val)); break
    except Exception as e:
        print("  miss str", repr(val), str(e)[:80])
if not ok:
    # Try via loaded UEnum byte value
    try:
        en = unreal.load_asset("/Game/CharacterCustomizer/CharacterCustomizer_Core/Data/Character_Load_Option")
        print("enum asset:", en, type(en).__name__)
        # UserDefinedEnum: get number of enums and display names
        n = en.num_enums() if hasattr(en, "num_enums") else None
        print("num_enums:", n)
    except Exception as e:
        print("enum load err", e)

# Read back
try:
    print("CLO now:", a.get_editor_property("Character Load Option"))
except Exception as e:
    print("readback err", e)

# Force construction script rerun by nudging transform
loc = a.get_actor_location()
a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 1.0), False, False)
a.set_actor_location(loc, False, False)
print("nudged; check viewport for assembled Marcus")
