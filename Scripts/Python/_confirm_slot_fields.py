import unreal
ok = False
for name in ["FFItemSlot", "FItemSlot", "ItemSlot"]:
    cls = getattr(unreal, name, None)
    if cls is None:
        continue
    try:
        inst = cls()
    except Exception as e:
        print(name, "exists but cannot instantiate:", e); continue
    print("=== %s instance attrs ===" % name)
    for a in sorted(dir(inst)):
        if a.startswith("_") or a[0].isupper():
            continue
        try:
            v = getattr(inst, a)
            if callable(v):
                continue
            print("   %-22s = %r" % (a, v))
        except Exception:
            pass
    ok = True
    break
if not ok:
    print("Slot struct NOT exposed to Python — relying on C++ substring match (Item / Amount) + runtime log.")
print("DONE")
