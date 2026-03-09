"""
Generate 6 floor number sign textures — number fills entire texture, no margin.
Fully opaque sign, no alpha fade issues with decals.
"""

from PIL import Image, ImageDraw, ImageFont
import os

OUTPUT_DIR = "C:/Users/Ommei/workspace/TheSignal/RawContent/Textures/FloorSigns"
os.makedirs(OUTPUT_DIR, exist_ok=True)

SIZE = 512  # Smaller texture — floor signs don't need 2K
BG_COLOR = (30, 30, 35, 255)       # Fully opaque dark background
BOX_COLOR = (200, 200, 200, 255)
NUM_COLOR = (240, 240, 240, 255)
BOX_THICKNESS = 8

font = None
for fp in ["C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/arial.ttf"]:
    if os.path.exists(fp):
        font = ImageFont.truetype(fp, 360)
        break

for num in range(1, 7):
    img = Image.new("RGBA", (SIZE, SIZE), BG_COLOR)
    draw = ImageDraw.Draw(img)

    # Box outline fills entire image edge-to-edge
    draw.rectangle([4, 4, SIZE - 5, SIZE - 5], outline=BOX_COLOR, width=BOX_THICKNESS)

    # Number centered
    text = str(num)
    bbox = draw.textbbox((0, 0), text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    tx = (SIZE - tw) / 2 - bbox[0]
    ty = (SIZE - th) / 2 - bbox[1]
    draw.text((tx, ty), text, fill=NUM_COLOR, font=font)

    out_path = os.path.join(OUTPUT_DIR, f"T_FloorSign_{num}.png")
    img.save(out_path, "PNG")
    print(f"Created: T_FloorSign_{num}.png")

print("Done!")
