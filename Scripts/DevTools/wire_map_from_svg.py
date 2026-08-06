"""
The Signal - wire an authored floor-plan SVG into the game as a map texture.

Takes an SVG saved by Dev Tools (Scripts/DevTools/app.py), renders it to a PNG, imports
that PNG as a UE texture, and assigns it to a named ZP_MapPickup together with the exact
world bounds the image covers.

    python Scripts/DevTools/wire_map_from_svg.py

Requires the editor to be open (BlueprintMCP on :9847). Re-run it any time an SVG changes.

WHY THE BOUNDS MATTER
    UZP_MapWidget::WorldToMapUV does  UV = (WorldXY - BoundsMin) / (BoundsMax - BoundsMin)
    and then draws the player marker at  UV * CanvasSize.  The texture fills that canvas,
    so the image's four corners must correspond EXACTLY to BoundsMin/BoundsMax or the
    marker drifts. We therefore declare the bounds of the whole SVG CANVAS (which includes
    the 100 UU margin app.py writes around the geometry) rather than the geometry bounds -
    image edge and declared bound are then the same thing by construction.

RECOVERING WORLD SPACE FROM AN SVG
    app.py's _save_svg writes at 1:1 scale with a 100 unit margin:
        svg_x = (world_x - min_x) + 100
    min_x is not stored, but the grid lines are drawn at world multiples of GRID_STEP, and
    the first one sits at  trunc(min_x / GRID_STEP) * GRID_STEP.  That fixes min_x modulo
    GRID_STEP; the caller's expected_bounds hint resolves which multiple it is. The result
    is cross-checked against the hint and the script refuses to run if they disagree.
"""

import json
import math
import os
import re
import sys
import urllib.request

MCP_URL = "http://localhost:9847/api/python"
HERE = os.path.dirname(os.path.abspath(__file__))
PLANS_DIR = os.path.join(HERE, "..", "FloorPlans")
GRID_STEP = 1000.0      # must match _save_svg's grid_step
SVG_PADDING = 100.0     # must match _save_svg's padding
TARGET_MAX_PX = 2048    # long edge of the exported texture
DEST_PACKAGE = "/Game/Core/Maps"

# --- what to wire -----------------------------------------------------------------
# expected_bounds is the geometry extent Dev Tools reported for that floor; it only has to
# be within a few hundred UU - it just disambiguates which multiple of GRID_STEP to use.
MAPS = [
    {
        "svg": "Main Floor.svg",
        "pickup": "ZP_MapPickup",
        "area_id": "RF_MainFloor",
        "display_name": "Main Floor",
        "asset": "T_Map_RF_MainFloor",
        "expected_bounds": (-4174.0, -3966.0, 4360.0, 6758.0),
    },
    {
        "svg": "Sub Basement.svg",
        "pickup": "ZP_MapPickup2",
        "area_id": "RF_SubBasement",
        "display_name": "Sub Basement",
        "asset": "T_Map_RF_SubBasement",
        "expected_bounds": (-4150.0, -250.0, 1548.0, 6710.0),
    },
]

# Chrome that app.py stamps into every SVG for the desktop editor. It has no business on
# an in-game map, so it is dropped: grid lines, the scale bar, its "10m" caption and the
# "Floor N" title. Anything else the dev draws or types IS rendered.
CHROME_TEXT_EXACT = {"10m"}
CHROME_TEXT_PREFIX = ("Floor ",)
CHROME_TITLE_MAX_Y = 25.0


def run_ue_python(code, timeout=300):
    req = urllib.request.Request(
        MCP_URL,
        data=json.dumps({"code": code}).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def ue_log(result):
    return [e.get("message", "").rstrip() for e in result.get("log", [])]


# ------------------------------------------------------------------ SVG -> geometry

def parse_svg(path):
    with open(path, encoding="utf-8") as f:
        svg = f.read()

    m = re.search(r'<svg[^>]*width="([\d.]+)"[^>]*height="([\d.]+)"', svg)
    if not m:
        raise ValueError("%s: no <svg> width/height" % path)
    canvas_w, canvas_h = float(m.group(1)), float(m.group(2))

    polys = []
    for pm in re.finditer(r'<polygon\s+([^>]*?)/?>', svg):
        attrs = pm.group(1)
        pts_m = re.search(r'points="([^"]+)"', attrs)
        if not pts_m:
            continue
        pts = []
        for pair in pts_m.group(1).split():
            if "," in pair:
                x, y = pair.split(",")
                pts.append((float(x), float(y)))
        if len(pts) < 3:
            continue
        fill = re.search(r'fill="([^"]+)"', attrs)
        stroke = re.search(r'stroke="([^"]+)"', attrs)
        polys.append({
            "pts": pts,
            "fill": fill.group(1) if fill else "#555555",
            "stroke": stroke.group(1) if stroke else None,
        })

    texts = []
    for tm in re.finditer(r'<text\s+([^>]*)>([^<]*)</text>', svg):
        attrs, body = tm.group(1), tm.group(2)
        x = re.search(r'x="([-\d.]+)"', attrs)
        y = re.search(r'y="([-\d.]+)"', attrs)
        if not (x and y):
            continue
        yv = float(y.group(1))
        if body in CHROME_TEXT_EXACT:
            continue
        if yv <= CHROME_TITLE_MAX_Y and body.startswith(CHROME_TEXT_PREFIX):
            continue
        fs = re.search(r'font-size="([-\d.]+)"', attrs)
        fill = re.search(r'fill="([^"]+)"', attrs)
        texts.append({
            "x": float(x.group(1)), "y": yv, "body": body,
            "size": float(fs.group(1)) if fs else 12.0,
            "fill": fill.group(1) if fill else "#ffffff",
        })

    # first grid line on each axis -> recovers the world origin (see module docstring)
    vfirst = re.search(r'<line x1="([\d.]+)" y1="0"', svg)
    hfirst = re.search(r'<line x1="0" y1="([\d.]+)"', svg)
    if not (vfirst and hfirst):
        raise ValueError("%s: no grid lines - cannot recover world origin" % path)

    return {
        "canvas": (canvas_w, canvas_h),
        "polys": polys,
        "texts": texts,
        "grid_first": (float(vfirst.group(1)), float(hfirst.group(1))),
    }


def recover_world_min(svg_grid_pos, hint_min):
    """svg_grid_pos = where the first grid line landed. hint_min = approximate world min."""
    phase = svg_grid_pos - SVG_PADDING          # = grid_world - world_min, in [0, GRID_STEP)
    grid_world = math.trunc(hint_min / GRID_STEP) * GRID_STEP
    return grid_world - phase


def svg_world_rect(info, expected):
    (cw, ch) = info["canvas"]
    gx, gy = info["grid_first"]
    exp_min_x, exp_min_y, exp_max_x, exp_max_y = expected

    min_x = recover_world_min(gx, exp_min_x)
    min_y = recover_world_min(gy, exp_min_y)
    max_x = min_x + (cw - 2 * SVG_PADDING)
    max_y = min_y + (ch - 2 * SVG_PADDING)

    for got, want, label in ((min_x, exp_min_x, "min_x"), (min_y, exp_min_y, "min_y"),
                             (max_x, exp_max_x, "max_x"), (max_y, exp_max_y, "max_y")):
        if abs(got - want) > 2.0:
            raise ValueError(
                "recovered %s = %.1f but Dev Tools reported %.1f. The SVG does not match "
                "expected_bounds - re-scan the floor and update MAPS." % (label, got, want))

    # The IMAGE covers the whole canvas, i.e. the geometry rect grown by SVG_PADDING.
    return (min_x - SVG_PADDING, min_y - SVG_PADDING,
            max_x + SVG_PADDING, max_y + SVG_PADDING)


# ------------------------------------------------------------------ render

def render_png(info, out_path):
    from PIL import Image, ImageDraw, ImageFont

    cw, ch = info["canvas"]
    scale = TARGET_MAX_PX / max(cw, ch)
    img_w, img_h = max(1, int(round(cw * scale))), max(1, int(round(ch * scale)))

    img = Image.new("RGBA", (img_w, img_h), (26, 26, 26, 255))
    draw = ImageDraw.Draw(img)

    def hex_to_rgb(h):
        h = (h or "").lstrip("#")
        if len(h) == 3:
            h = "".join(c * 2 for c in h)
        if len(h) != 6:
            return (255, 255, 255)
        return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

    for p in info["polys"]:
        pts = [(x * scale, y * scale) for x, y in p["pts"]]
        outline = hex_to_rgb(p["stroke"]) if p["stroke"] else None
        draw.polygon(pts, fill=hex_to_rgb(p["fill"]), outline=outline)

    for t in info["texts"]:
        size = max(8, int(round(t["size"] * scale)))
        font = None
        for name in ("segoeui.ttf", "arial.ttf", "DejaVuSans.ttf"):
            try:
                font = ImageFont.truetype(name, size)
                break
            except OSError:
                continue
        if font is None:
            font = ImageFont.load_default()
        draw.text((t["x"] * scale, t["y"] * scale), t["body"],
                  fill=hex_to_rgb(t["fill"]), font=font, anchor="mm")

    img.save(out_path, "PNG")
    return img_w, img_h


# ------------------------------------------------------------------ UE side

UE_SCRIPT = r'''
import unreal

png_path      = r"__PNG__"
asset_name    = "__ASSET__"
dest_path     = "__DEST__"
pickup_label  = "__PICKUP__"
area_id       = "__AREAID__"
display_name  = "__DISPLAY__"
bmin_x, bmin_y, bmax_x, bmax_y = __BOUNDS__

if not unreal.EditorAssetLibrary.does_directory_exist(dest_path):
    unreal.EditorAssetLibrary.make_directory(dest_path)

task = unreal.AssetImportTask()
task.set_editor_property("filename", png_path)
task.set_editor_property("destination_path", dest_path)
task.set_editor_property("destination_name", asset_name)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

tex = unreal.load_asset(dest_path + "/" + asset_name)
if not tex:
    print("WIRE_FAIL: could not import " + asset_name)
else:
    # UI texture: crisp, no mip blur in the map widget.
    try:
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("srgb", True)
    except Exception as e:
        print("WIRE_WARN: texture settings: " + str(e))
    unreal.EditorAssetLibrary.save_asset(dest_path + "/" + asset_name)

    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    hits = 0
    for a in eas.get_all_level_actors():
        if "MapPickup" not in a.get_class().get_name():
            continue
        if a.get_actor_label() != pickup_label:
            continue
        a.set_editor_property("AZP_MapTexture", tex)
        a.set_editor_property("AZP_AreaID", area_id)
        a.set_editor_property("AZP_AreaDisplayName", unreal.Text(display_name))
        a.set_editor_property("AZP_MapBoundsMin", unreal.Vector2D(bmin_x, bmin_y))
        a.set_editor_property("AZP_MapBoundsMax", unreal.Vector2D(bmax_x, bmax_y))
        hits += 1
        loc = a.get_actor_location()
        print("WIRE_OK: %s <- %s  areaID=%s  bounds=(%.1f,%.1f)..(%.1f,%.1f)  pickupZ=%.0f"
              % (pickup_label, asset_name, area_id, bmin_x, bmin_y, bmax_x, bmax_y, loc.z))
    if hits == 0:
        print("WIRE_FAIL: no MapPickup labelled '" + pickup_label + "' in this level")
    elif hits > 1:
        print("WIRE_WARN: %d actors share the label '%s'" % (hits, pickup_label))
'''


def main():
    plans = os.path.abspath(PLANS_DIR)
    failures = []

    for entry in MAPS:
        svg_path = os.path.join(plans, entry["svg"])
        print("=" * 72)
        print(entry["svg"], "->", entry["pickup"])
        if not os.path.isfile(svg_path):
            print("  SKIP: not found:", svg_path)
            failures.append(entry["svg"] + " (missing)")
            continue

        info = parse_svg(svg_path)
        bounds = svg_world_rect(info, entry["expected_bounds"])
        print("  canvas %.0f x %.0f, %d polygons, %d texts"
              % (info["canvas"][0], info["canvas"][1], len(info["polys"]), len(info["texts"])))
        print("  world bounds (%.1f, %.1f) .. (%.1f, %.1f)   span %.0f x %.0f"
              % (bounds[0], bounds[1], bounds[2], bounds[3],
                 bounds[2] - bounds[0], bounds[3] - bounds[1]))

        png_path = os.path.join(plans, entry["asset"] + ".png")
        w, h = render_png(info, png_path)
        print("  PNG %dx%d -> %s" % (w, h, png_path))

        code = (UE_SCRIPT
                .replace("__PNG__", png_path.replace("\\", "/"))
                .replace("__ASSET__", entry["asset"])
                .replace("__DEST__", DEST_PACKAGE)
                .replace("__PICKUP__", entry["pickup"])
                .replace("__AREAID__", entry["area_id"])
                .replace("__DISPLAY__", entry["display_name"])
                .replace("__BOUNDS__", "(%.1f, %.1f, %.1f, %.1f)" % bounds))

        tmp = os.path.join(plans, "_wire_%s.py" % entry["asset"])
        with open(tmp, "w", encoding="utf-8") as f:
            f.write(code)
        result = run_ue_python('exec(open(r"%s").read())' % tmp.replace("\\", "/"))
        os.remove(tmp)

        if not result.get("success"):
            print("  UE ERROR:", result.get("error"))
            failures.append(entry["svg"])
            continue
        for line in ue_log(result):
            if line.startswith(("WIRE_OK", "WIRE_FAIL", "WIRE_WARN")):
                print("  " + line)
                if line.startswith("WIRE_FAIL"):
                    failures.append(entry["svg"])

    print("=" * 72)
    print("Saving level...")
    res = run_ue_python(
        "import unreal; "
        "print('SAVED' if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)"
        ".save_current_level() else 'SAVE_FAILED')")
    for line in ue_log(res):
        print("  " + line)

    if failures:
        print("\nFAILED: " + ", ".join(failures))
        return 1
    print("\nAll maps wired.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
