from PIL import Image, ImageDraw
import os

# Create images directory
if not os.path.exists('miniprogram/images'):
    os.makedirs('miniprogram/images')

def create_icon(filename, color, type):
    # 81x81 transparent
    img = Image.new('RGBA', (81, 81), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Draw simple shapes based on type
    # Color hex to tuple
    c = tuple(int(color.lstrip('#')[i:i+2], 16) for i in (0, 2, 4)) + (255,)

    w, h = 81, 81
    padding = 15
    lw = 6

    if type == 'device':
        # Rectangle (Screen)
        draw.rectangle([padding, padding, w-padding, h-padding], outline=c, width=lw)
        # Antenna
        draw.line([w/2, padding, w/2, 5], fill=c, width=lw)

    elif type == 'focus':
        # Circle (Clock)
        draw.ellipse([padding, padding, w-padding, h-padding], outline=c, width=lw)
        # Hands
        draw.line([w/2, h/2, w/2, padding+10], fill=c, width=lw)
        draw.line([w/2, h/2, w-padding-10, h/2], fill=c, width=lw)

    elif type == 'stats':
        # Bar chart
        draw.rectangle([20, h-20, 30, h-padding], fill=c)
        draw.rectangle([35, h-40, 45, h-padding], fill=c)
        draw.rectangle([50, h-30, 60, h-padding], fill=c)
        # Axis
        draw.line([padding, h-padding, w-padding, h-padding], fill=c, width=lw)
        draw.line([padding, padding, padding, h-padding], fill=c, width=lw)

    img.save(f'miniprogram/images/{filename}')
    print(f"Created {filename}")

# Standard WeChat colors
# Normal: #8a8a8a, Active: #1296db (blue) or #3cc51f (green)
color_normal = '#8a8a8a'
color_active = '#1296db'

create_icon('device.png', color_normal, 'device')
create_icon('device-active.png', color_active, 'device')
create_icon('focus.png', color_normal, 'focus')
create_icon('focus-active.png', color_active, 'focus')
create_icon('stats.png', color_normal, 'stats')
create_icon('stats-active.png', color_active, 'stats')

