import unreal
so = unreal.GameplayStatics.load_game_from_slot("CC_SaveGame", 0)
m = so.get_editor_property("saved_characters")
print("MAP TYPE", type(m).__name__)
keys = list(m.keys())
print("KEYS", [str(k) for k in keys])

# Find Marcus (case-insensitive)
target = None
for k in keys:
    if str(k).lower() == "marcus":
        target = k
        break
if target is None and keys:
    target = keys[0]
print("USING KEY", str(target))

val = m[target]
print("VALUE TYPE", type(val).__name__)

def dump_struct(s, prefix=""):
    # Try to enumerate struct fields by exporting to text
    try:
        txt = s.export_text() if hasattr(s, "export_text") else None
    except Exception:
        txt = None
    if txt:
        print(prefix + "EXPORT:", txt[:1500])
    # Try common CC field names
    for f in ["body","head","body_mesh","head_mesh","skeletal_mesh","mesh","gender","sex",
              "ethnicity","age_group","outfit","apparel","apparels","apparel_array","clothing",
              "materials","material_data","skin","hair","morphs","morph_targets","body_type",
              "height","preset","name","character_name","outfit_definition","equipped"]:
        try:
            fv = s.get_editor_property(f)
            if isinstance(fv, (list, set)):
                print(prefix + "FIELD", f, "=> list len", len(fv))
                for i, e in enumerate(fv[:8]):
                    print(prefix + "   [%d]" % i, str(e)[:200])
            else:
                print(prefix + "FIELD", f, "=>", type(fv).__name__, str(fv)[:200])
        except Exception:
            pass

dump_struct(val)
