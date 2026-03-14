import unreal
from collections import defaultdict

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Scan F1 (Z 0-100) as representative sample — categorize by mesh name
mesh_counts = defaultdict(int)
mesh_samples = defaultdict(list)
bp_counts = defaultdict(int)

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < -50 or loc.z > 100:  # F1 only
        continue

    cn = a.get_class().get_name()

    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            sm = comps[0].get_editor_property("static_mesh")
            mesh_name = sm.get_name() if sm else "None"
            mesh_counts[mesh_name] += 1
            if len(mesh_samples[mesh_name]) < 2:
                mesh_samples[mesh_name].append(a.get_name())
    else:
        bp_counts[cn] += 1

# Categorize meshes into KEEP vs DELETE
keep_keywords = [
    "Wall", "wall", "Ceiling", "ceiling", "Floor", "floor",
    "Window", "window", "Glass", "glass",
    "Elevator", "elevator", "Stair", "stair", "Step", "step",
    "Vent", "vent", "AC_", "Pipe", "pipe",
    "Light", "light", "Lamp", "lamp", "Florosent", "Neon",
    "Frame", "frame", "Pillar", "pillar", "Column", "column",
    "Exit", "exit", "Door", "door",
    "Cube",  # structural cubes (walls/floors)
    "Cylinder",  # structural columns
    "Baseboard", "baseboard", "Molding", "molding",
    "Fixture", "fixture", "Switch", "switch",
    "FireExtinguisher", "Sprinkler", "Alarm",
    "Sign", "sign", "Plaque",
    "OutDoorUnit",  # HVAC units
]

delete_keywords = [
    "Chair", "chair", "Desk", "desk", "Table", "table",
    "Sofa", "sofa", "Couch", "couch",
    "Monitor", "monitor", "Screen", "screen", "Computer", "computer",
    "Phone", "phone", "Keyboard", "keyboard",
    "Book", "book", "Paper", "paper", "Folder", "folder",
    "Plant", "plant", "Pot", "pot", "Flower",
    "Shelf", "shelf", "Cabinet", "cabinet", "Drawer", "drawer",
    "Printer", "printer", "Copier",
    "Clock", "clock", "Calendar",
    "Cup", "cup", "Mug", "mug", "Bottle", "bottle",
    "Trash", "trash", "Bin", "bin",
    "Board", "board", "Whiteboard",
    "Projector", "projector",
    "Coffee", "coffee", "Vending", "vending",
    "Fridge", "fridge", "Microwave", "microwave",
    "Stapler", "stapler", "Pen", "pen",
    "Frame_picture", "Photo", "photo", "Painting",
    "Cushion", "cushion", "Pillow",
    "Carpet", "carpet", "Rug", "rug",
    "Rack", "rack",
    "vert", "horz",  # floor plan grid lines
    "Room",  # full room meshes (ConferenceSecretary, etc.)
    "Locker",  # furniture lockers (not loot lockers)
    "TV", "tv_",
    "Fax", "Scanner",
]

keep = {}
delete = {}
unknown = {}

for mesh_name, count in sorted(mesh_counts.items(), key=lambda x: -x[1]):
    categorized = False

    for k in delete_keywords:
        if k in mesh_name:
            delete[mesh_name] = count
            categorized = True
            break

    if not categorized:
        for k in keep_keywords:
            if k in mesh_name:
                keep[mesh_name] = count
                categorized = True
                break

    if not categorized:
        unknown[mesh_name] = count

print("=== F1 SCAN — KEEP (walls, windows, fixtures, elevator, structural) ===")
keep_total = 0
for name, count in sorted(keep.items(), key=lambda x: -x[1]):
    print(f"  {count:4d}x {name}")
    keep_total += count
print(f"  TOTAL KEEP: {keep_total}\n")

print("=== F1 SCAN — DELETE (furniture, props, decor, floor plan lines) ===")
del_total = 0
for name, count in sorted(delete.items(), key=lambda x: -x[1]):
    print(f"  {count:4d}x {name}")
    del_total += count
print(f"  TOTAL DELETE: {del_total}\n")

print("=== F1 SCAN — UNKNOWN (need your call) ===")
unk_total = 0
for name, count in sorted(unknown.items(), key=lambda x: -x[1]):
    samples = ", ".join(mesh_samples[name])
    print(f"  {count:4d}x {name}  (e.g. {samples})")
    unk_total += count
print(f"  TOTAL UNKNOWN: {unk_total}\n")

print("=== Blueprint actors on F1 ===")
for cn, count in sorted(bp_counts.items(), key=lambda x: -x[1]):
    print(f"  {count:4d}x {cn}")
