"""
Fix F5v4.svg — restore world UU coordinates.
Current coords: 50.4-57.6 X, 50.2-58.2 Y (7.2 x 8.0 range)
Target: building extent ~7600 x 8000 UU, centered at X=-1221, Y=1897
Scale factor: ~1000x (3 rounds of 0.1 shrinkage)
"""
import re

input_path = "C:/Users/Ommei/workspace/TheSignal/Scripts/FloorPlans/F5v4.svg"
output_path = "C:/Users/Ommei/workspace/TheSignal/Scripts/FloorPlans/F5v4_fixed.svg"

with open(input_path, 'r') as f:
    svg = f.read()

# First pass: find current extent
all_coords = []
for m in re.finditer(r'points="([^"]+)"', svg):
    for pair in m.group(1).split():
        x, y = pair.split(',')
        all_coords.append((float(x), float(y)))

cur_min_x = min(c[0] for c in all_coords)
cur_max_x = max(c[0] for c in all_coords)
cur_min_y = min(c[1] for c in all_coords)
cur_max_y = max(c[1] for c in all_coords)
cur_cx = (cur_min_x + cur_max_x) / 2
cur_cy = (cur_min_y + cur_max_y) / 2
cur_w = cur_max_x - cur_min_x
cur_h = cur_max_y - cur_min_y

print(f"Current: {cur_min_x:.1f} to {cur_max_x:.1f} x {cur_min_y:.1f} to {cur_max_y:.1f}")
print(f"  Size: {cur_w:.1f} x {cur_h:.1f}, Center: ({cur_cx:.1f}, {cur_cy:.1f})")

# Target: building world coordinates (from UE5 scan)
# Building X: -5024 to 2581 (center -1221)
# Building Y: -1750 to 5545 for single floor (center 1897)
TARGET_CX = -1221.0
TARGET_CY = 1897.0
TARGET_W = 7605.0  # X range
TARGET_H = 7295.0  # Y range

# Uniform scale to fit
scale = min(TARGET_W / cur_w, TARGET_H / cur_h)
print(f"Scale factor: {scale:.0f}x")

def transform(sx, sy):
    wx = (sx - cur_cx) * scale + TARGET_CX
    wy = (sy - cur_cy) * scale + TARGET_CY
    return wx, wy

# Transform polygon points
def fix_points(match):
    pts_str = match.group(1)
    new_pts = []
    for pair in pts_str.split():
        x, y = pair.split(',')
        nx, ny = transform(float(x), float(y))
        new_pts.append(f"{nx:.1f},{ny:.1f}")
    return f'points="{" ".join(new_pts)}"'

svg = re.sub(r'points="([^"]+)"', fix_points, svg)

# Transform line coordinates
def fix_line(match):
    full = match.group(0)
    x1, y1 = transform(float(re.search(r'x1="([^"]+)"', full).group(1)),
                        float(re.search(r'y1="([^"]+)"', full).group(1)))
    x2, y2 = transform(float(re.search(r'x2="([^"]+)"', full).group(1)),
                        float(re.search(r'y2="([^"]+)"', full).group(1)))
    rest = re.sub(r'x1="[^"]+"', f'x1="{x1:.1f}"', full)
    rest = re.sub(r'y1="[^"]+"', f'y1="{y1:.1f}"', rest)
    rest = re.sub(r'x2="[^"]+"', f'x2="{x2:.1f}"', rest)
    rest = re.sub(r'y2="[^"]+"', f'y2="{y2:.1f}"', rest)
    return rest

svg = re.sub(r'<line[^/]+/>', fix_line, svg)

# Transform text positions
def fix_text(match):
    full = match.group(0)
    x_m = re.search(r'\bx="([^"]+)"', full)
    y_m = re.search(r'\by="([^"]+)"', full)
    if x_m and y_m:
        nx, ny = transform(float(x_m.group(1)), float(y_m.group(1)))
        full = re.sub(r'\bx="[^"]+"', f'x="{nx:.1f}"', full, count=1)
        full = re.sub(r'\by="[^"]+"', f'y="{ny:.1f}"', full, count=1)
    return full

svg = re.sub(r'<text[^>]*>[^<]*</text>', fix_text, svg)

# Update SVG dimensions
new_w = int(TARGET_W + 200)
new_h = int(TARGET_H + 200)
svg = re.sub(r'width="\d+"', f'width="{new_w}"', svg)
svg = re.sub(r'height="\d+"', f'height="{new_h}"', svg)

# Verify
verify_coords = []
for m in re.finditer(r'points="([^"]+)"', svg):
    for pair in m.group(1).split():
        x, y = pair.split(',')
        verify_coords.append((float(x), float(y)))

vxs = [c[0] for c in verify_coords]
vys = [c[1] for c in verify_coords]
print(f"\nRestored: X {min(vxs):.0f} to {max(vxs):.0f}, Y {min(vys):.0f} to {max(vys):.0f}")
print(f"  Size: {max(vxs)-min(vxs):.0f} x {max(vys)-min(vys):.0f} UU")
print(f"  SVG: {new_w} x {new_h}")
print(f"\nSaved: {output_path}")

with open(output_path, 'w') as f:
    f.write(svg)
