#!/usr/bin/env python3
"""
OLED 128x64 Layout Visualizer
Simulates the ESP32 navigation display layout without uploading to hardware
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Screen dimensions
WIDTH = 128
HEIGHT = 64

# Sample navigation data - EDIT THESE to test different layouts
nav_data = {
    'title': '250m',           # Distance to next turn (top left)
    'eta': '10:45 AM',         # ETA (top right)
    'duration': '5 min',       # Duration (below ETA)
    'distance': '2.3 km',      # Total distance (below duration)
    'directions': 'Turn left onto Main Street',  # Bottom line
    'active': True,
    'isNavigation': True
}

def create_oled_display(data):
    """Create a visual representation of the OLED display"""
    
    # Create black background (OLED style)
    img = Image.new('1', (WIDTH, HEIGHT), 0)  # 1-bit black and white
    draw = ImageDraw.Draw(img)
    
    # Load fonts with different sizes
    try:
        # Try to load TrueType font (adjust size here: 10, 12, 16, 20, etc.)
        font_small = ImageFont.truetype("arial.ttf", 8)   # Small text
        font_medium = ImageFont.truetype("arial.ttf", 12)  # Medium text
        font_large = ImageFont.truetype("arial.ttf", 16)   # Large text (like title)
    except:
        try:
            # Fallback to default font
            font_small = ImageFont.load_default()
            font_medium = ImageFont.load_default()
            font_large = ImageFont.load_default()
        except:
            font_small = font_medium = font_large = None
    
    if data['active'] and data['isNavigation']:
        # TITLE (top left, y=0) - LARGE font
        title_text = data['title'][:7] if data['title'] else ''
        draw.text((70, 0), title_text, fill=1, font=font_large)
        
        # ETA (top right, x=70, y=0) - SMALL font
        eta_text = data['eta'][:8] if data['eta'] else ''
        draw.text((0, 0), eta_text, fill=1, font=font_small)
        
        # DURATION (below ETA, x=70, y=10) - SMALL font
        duration_text = data['duration'][:8] if data['duration'] else ''
        draw.text((70, 30), duration_text, fill=1, font=font_small)
        
        # DISTANCE (below duration, x=70, y=20) - SMALL font
        distance_text = data['distance'][:8] if data['distance'] else ''
        draw.text((70, 40), distance_text, fill=1, font=font_small)
        
        # ICON placeholder (centered 48x48, x=40, y=8)
        icon_x = 0
        icon_y = 8
        # Draw rectangle to represent icon
        draw.rectangle([icon_x, icon_y, icon_x+48, icon_y+48], outline=1, width=1)
        draw.text((icon_x+10, icon_y+20), "ICON", fill=1, font=font_small)
        draw.text((icon_x+8, icon_y+30), "48x48", fill=1, font=font_small)
        
        # DIRECTIONS (bottom line, y=56) - SMALL font
        directions_text = data['directions']
        if len(directions_text) > 21:
            directions_text = directions_text[:18] + "..."
        draw.text((0, 56), directions_text, fill=1, font=font_small)
    else:
        # Show "No Navigation" message
        draw.text((10, 24), "No Navigation", fill=1, font=font_medium)
        draw.text((20, 36), "Active", fill=1, font=font_medium)
    
    return img

def print_ascii_preview(data):
    """Print ASCII art preview to console"""
    print("\n" + "="*50)
    print("OLED 128x64 Preview".center(50))
    print("="*50)
    
    if data['active'] and data['isNavigation']:
        title = data['title'][:7] if data['title'] else '---'
        eta = data['eta'][:8] if data['eta'] else '---'
        duration = data['duration'][:8] if data['duration'] else '---'
        distance = data['distance'][:8] if data['distance'] else '---'
        directions = data['directions']
        if len(directions) > 21:
            directions = directions[:18] + "..."
        
        print(f"┌{'─'*46}┐")
        print(f"│ {title:<25} {eta:>18} │")  # y=0
        print(f"│ {'':<25} {duration:>18} │")  # y=10
        print(f"│ {'':<25} {distance:>18} │")  # y=20
        print(f"│           ┌────────┐                   │")
        print(f"│           │  ICON  │                   │")  # y=8-56
        print(f"│           │ 48x48  │                   │")
        print(f"│           │ @40,8  │                   │")
        print(f"│           └────────┘                   │")
        print(f"│                                        │")
        print(f"│ {directions:<44} │")  # y=56
        print(f"└{'─'*46}┘")
    else:
        print(f"┌{'─'*46}┐")
        print(f"│{'':^46}│")
        print(f"│{'No Navigation Active':^46}│")
        print(f"│{'':^46}│")
        print(f"└{'─'*46}┘")
    
    print("="*50 + "\n")

def main():
    print("ESP32 OLED Navigation Display Visualizer")
    print("-" * 50)
    
    # Print ASCII preview
    print_ascii_preview(nav_data)
    
    # Create and save image
    img = create_oled_display(nav_data)
    
    # Scale up for better visibility (4x)
    img_scaled = img.resize((WIDTH*4, HEIGHT*4), Image.NEAREST)
    
    # Save as PNG
    output_file = "oled_preview.png"
    img_scaled.save(output_file)
    print(f"✓ Image saved as: {output_file}")
    print(f"  (Scaled 4x for visibility: {WIDTH*4}x{HEIGHT*4})")
    
    # Try to open the image
    try:
        if os.name == 'nt':  # Windows
            os.startfile(output_file)
        elif os.name == 'posix':  # macOS/Linux
            os.system(f'open {output_file} || xdg-open {output_file}')
        print(f"✓ Opening preview...")
    except:
        print(f"  → Manually open '{output_file}' to view")

if __name__ == "__main__":
    main()
    
    print("\n" + "="*50)
    print("HOW TO USE:")
    print("="*50)
    print("1. Edit the 'nav_data' dictionary at the top of this file")
    print("2. Run: python oled_visualizer.py")
    print("3. Check console ASCII preview + generated PNG image")
    print("="*50)
