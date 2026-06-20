import unreal

def tracks(path):
    a = unreal.load_asset(path)
    if not a:
        print("MISSING:", path); return set()
    try:
        names = unreal.AnimationLibrary.get_animation_track_names(a)
        return set(str(n) for n in names)
    except Exception as e:
        print("track err for", path, ":", e); return set()

idle  = tracks("/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle.A_MeleePipe_Idle")
block = tracks("/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockLoop.FPP_Longs_BlockLoop")

def hl(s):
    return sorted([b for b in s if any(k in b.lower() for k in ["hand","weapon","gun","ik_","grip","finger","thumb","index","middle","ring","pinky"])])

print("IDLE track count:", len(idle))
print("BLOCK track count:", len(block))
print("IDLE hand/weapon tracks:", hl(idle))
print("BLOCK hand/weapon tracks:", hl(block))
print("IN IDLE NOT BLOCK:", sorted(idle - block)[:40])
print("IN BLOCK NOT IDLE:", sorted(block - idle)[:40])
