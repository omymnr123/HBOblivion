import sys
import io
import math
sys.path.append(r"d:\HB Server\Helbreath-Heldenian-Project-Development\tools\MapRenderer")
from paklib import PAKFile, Sprite, SpriteRectangle
from PIL import Image

def resize_pak(filepath, scale):
    pak = PAKFile.read(filepath)
    new_pak = PAKFile()
    for sprite in pak.sprites:
        img = sprite.get_image()
        new_width = int(img.width * scale)
        new_height = int(img.height * scale)
        # Resize image using high-quality filter
        new_img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
        
        # Scale rectangles
        new_rects = []
        for r in sprite.rectangles:
            new_r = SpriteRectangle(
                x=int(r.x * scale),
                y=int(r.y * scale),
                width=int(r.width * scale),
                height=int(r.height * scale),
                pivot_x=int(r.pivot_x * scale),
                pivot_y=int(r.pivot_y * scale)
            )
            new_rects.append(new_r)
            
        new_pak.add_sprite(Sprite.from_image(new_img, new_rects))
        
    # Save back
    new_pak.write(filepath)
    print(f"Resized {filepath} successfully.")

pak_files = [
    r"d:\HB Server\Helbreath-Heldenian-Project-Development\Binaries\Game\sprites\npcs\Sarcofago_1.pak",
    r"d:\HB Server\Helbreath-Heldenian-Project-Development\Binaries\Game\sprites\npcs\Sarcofago_2.pak",
    r"d:\HB Server\Helbreath-Heldenian-Project-Development\Binaries\Game\sprites\npcs\Sarcofago_3.pak"
]

scale_factor = 0.85 # Reduce size by 15%

for p in pak_files:
    resize_pak(p, scale_factor)
