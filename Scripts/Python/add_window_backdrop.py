import unreal, random
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
random.seed(1771)

BP = unreal.load_asset("/Game/BackgroundBuildings/Blueprint/B_LowPolyBuilding")

# remove any prior run's backdrop so we don't stack duplicates (our own actors, safe)
old = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("Backdrop_Building_")]
for a in old: eas.destroy_actor(a)
if old: print("cleared previous backdrop:", len(old))

# Building footprint sits roughly x[-4500..+2000], y[-1900..+6900]. Place a city OUTSIDE,
# concentrated where the remaining windows look (west facade + S/N exit doors).
spots = []
# West rows (main glass facade faces -X) — two depth layers across full Y span
for y in range(-4000, 10001, 2100):
    spots.append((-11000, y))
for y in range(-3000, 9001, 2600):
    spots.append((-16500, y))
# South (exit door y=-1859 faces -Y)
for x in range(-6000, 3001, 2200):
    spots.append((x, -9500))
# North (exit door y=6909 faces +Y)
for x in range(-6000, 3001, 2200):
    spots.append((x, 13500))
# East (a little, for any east-facing glass / depth)
for y in range(-2000, 9001, 3000):
    spots.append((9000, y))

n = 0
for (x, y) in spots:
    jx = random.uniform(-500, 500); jy = random.uniform(-500, 500)
    sxy = random.uniform(2.6, 4.2)
    sz  = random.uniform(3.0, 6.5)
    yaw = random.choice([0, 90, 180, 270]) + random.uniform(-8, 8)
    loc = unreal.Vector(x + jx, y + jy, -2200.0)   # base below view, rises up past windows
    act = eas.spawn_actor_from_object(BP, loc, unreal.Rotator(0, 0, yaw))
    if not act:
        continue
    act.set_actor_scale3d(unreal.Vector(sxy, sxy, sz))
    n += 1
    act.set_actor_label("Backdrop_Building_%02d" % n)

print("backdrop buildings placed:", n)
print("level saved:", les.save_current_level())
