"""Adapted from recolor_icons.py for The Signal's weapon icons.
All weapon icons -> orange #FF6600 (matches DA_AssualtRifle), transparent bg.
- AssaultRifle / PoliceShotgun: already orange -> recolor RGB, keep alpha shape.
- Explosive: black line-art w/ alpha -> recolor RGB to orange, keep alpha shape.
- Pipe: black-on-white JPG -> derive alpha from line darkness, recolor, upscale.
Outputs to Icons/Recolored/.
"""
import os
from PIL import Image

BASE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(BASE, "Recolored")
os.makedirs(OUT, exist_ok=True)
ORANGE = (255, 102, 0)

def recolor_keep_alpha(src, dst):
    """Repaint RGB to orange, keep the existing alpha silhouette."""
    im = Image.open(src).convert("RGBA")
    out = [(ORANGE[0], ORANGE[1], ORANGE[2], a) for (_, _, _, a) in im.getdata()]
    im.putdata(out)
    im.save(dst, "PNG")
    print("  recolor+alpha:", os.path.basename(dst), im.size)

def from_white_bg(src, dst, cutoff=180, upscale=4, over_white=False):
    """Black line-art -> orange on transparent. Alpha from line darkness.
    over_white: composite over white first (for PNGs with an opaque white fill)."""
    im = Image.open(src).convert("RGBA")
    if over_white:
        bg = Image.new("RGBA", im.size, (255, 255, 255, 255))
        bg.alpha_composite(im)
        im = bg
    g = im.convert("L")
    if upscale > 1:
        g = g.resize((g.width * upscale, g.height * upscale), Image.LANCZOS)
    px = list(g.getdata())
    out = []
    for lum in px:
        a = 0 if lum >= cutoff else int(255 * (cutoff - lum) / cutoff)
        out.append((ORANGE[0], ORANGE[1], ORANGE[2], a))
    im = Image.new("RGBA", g.size)
    im.putdata(out)
    im.save(dst, "PNG")
    print("  white->transparent:", os.path.basename(dst), im.size)

print("Processing weapon icons -> #FF6600 on transparent:")
recolor_keep_alpha(os.path.join(BASE, "DA_AssualtRifle.png"),  os.path.join(OUT, "DA_AssualtRifle.png"))
recolor_keep_alpha(os.path.join(BASE, "DA_PolicShotgun.png"),  os.path.join(OUT, "DA_PolicShotgun.png"))
from_white_bg(os.path.join(BASE, "DA_Explosive.png"), os.path.join(OUT, "DA_Explosive.png"), cutoff=160, upscale=2, over_white=True)
from_white_bg(os.path.join(BASE, "DA_Pipe.jpg"),               os.path.join(OUT, "DA_Pipe.png"))
print("Done -> Icons/Recolored/")
