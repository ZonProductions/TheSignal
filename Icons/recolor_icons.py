"""
Recolor icon PNGs: recolors opaque pixels to the target color, preserves transparency.
Source icons should be colored outlines on transparent backgrounds.

Usage: python recolor_icons.py
Edit the ICONS list below to add/change icons and colors.
"""

import os
from PIL import Image

BASE = os.path.dirname(os.path.abspath(__file__))
OUT_BASE = os.path.join(BASE, 'Recolored')

# Define icon groups: (subfolder, filename, target_color_rgb)
ICONS = [
    # Merc icons -> orange #FF6600
    ('Merc', 'AR.png',        (0xFF, 0x66, 0x00)),
    ('Merc', 'Shotgun.png',   (0xFF, 0x66, 0x00)),
    ('Merc', 'Sniper.png',    (0xFF, 0x66, 0x00)),
    ('Merc', 'ProxyMine.png', (0xFF, 0x66, 0x00)),
    # Spy icons -> green #00FF41
    ('Spy', 'StunBullet.png', (0x00, 0xFF, 0x41)),
    ('Spy', 'StunMine.png',   (0x00, 0xFF, 0x41)),
    # Shared icons -> white #FFFFFF
    ('Shared', 'health.png',  (0xFF, 0xFF, 0xFF)),
]

ALPHA_THRESHOLD = 128

def process_icon(src_path, dst_path, color):
    img = Image.open(src_path).convert('RGBA')
    pixels = img.load()
    w, h = img.size

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a > ALPHA_THRESHOLD:
                # Opaque pixel -> recolor, preserve alpha
                pixels[x, y] = (color[0], color[1], color[2], a)
            else:
                # Transparent pixel -> keep transparent
                pixels[x, y] = (0, 0, 0, 0)

    img.save(dst_path, 'PNG')

def main():
    processed = 0
    for subfolder, filename, color in ICONS:
        src = os.path.join(BASE, subfolder, filename)
        dst_dir = os.path.join(OUT_BASE, subfolder)
        dst = os.path.join(dst_dir, filename)

        if not os.path.isfile(src):
            print(f'[SKIP] Source not found: {src}')
            continue

        os.makedirs(dst_dir, exist_ok=True)
        print(f'Processing: {subfolder}/{filename} -> color #{color[0]:02X}{color[1]:02X}{color[2]:02X}')
        process_icon(src, dst, color)
        print(f'  Saved: {dst}')
        processed += 1

    print(f'\nDone. {processed}/{len(ICONS)} icons processed.')

if __name__ == "__main__":
    main()
