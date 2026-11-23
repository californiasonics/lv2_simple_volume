"""
make_knob_strip.py - turn ONE picture of a knob into a "film strip" for MOD.

WHY A FILM STRIP?
-----------------
MOD doesn't rotate the knob picture itself. Instead it uses a strip of
pictures side by side, each showing the knob turned a little further,
like the frames of a cartoon flip-book:

    [ frame 0 ][ frame 1 ][ frame 2 ] ... [ frame 64 ]
     fully CCW                             fully CW

When you drag the knob, MOD slides the strip along so the right frame
shows through a square "window". Frame 0 = minimum (-40 dB), the last
frame = maximum (+6 dB).

MOD works out how many frames there are from the strip's size:
    number of frames = strip width / strip height
so every frame MUST be square.

HOW TO USE (in an Anaconda Prompt - Pillow comes with Anaconda):
    python make_knob_strip.py my_knob.png knob.png 108

    my_knob.png : your knob, square, pointer facing straight UP (12 o'clock),
                  transparent background
    knob.png    : the strip to put in lv2_simple_volume.lv2/modgui/
    108         : (optional) size of each frame on screen, in pixels.
                  Must match the knob's width and height in the stylesheet.
"""

import sys
from PIL import Image

FRAMES = 65          # number of positions; more = smoother turning
SIZE = 108           # default frame size in pixels (can be given on the command line)
SWEEP = 270          # total travel in degrees, like a real pot (7 to 5 o'clock)


def make_strip(knob_file, out_file, size):
    # Keep the knob at full resolution for now: rotating a big picture and
    # THEN shrinking it looks much sharper than the other way round.
    knob = Image.open(knob_file).convert("RGBA")

    # A blank, transparent strip wide enough for every frame
    strip = Image.new("RGBA", (size * FRAMES, size), (0, 0, 0, 0))

    for i in range(FRAMES):
        # Angle for this frame: from -135 deg (7 o'clock) to +135 deg (5 o'clock)
        angle = -SWEEP / 2 + SWEEP * i / (FRAMES - 1)
        # Pillow rotates anti-clockwise for positive angles, so flip the sign
        frame = knob.rotate(-angle, resample=Image.BICUBIC)
        frame = frame.resize((size, size), Image.LANCZOS)
        strip.paste(frame, (i * size, 0), frame)

    strip.save(out_file, optimize=True)
    print(f"Saved {out_file}: {FRAMES} frames of {size}x{size} px")


if __name__ == "__main__":
    if len(sys.argv) not in (3, 4):
        print(__doc__)
        sys.exit(1)
    size = int(sys.argv[3]) if len(sys.argv) == 4 else SIZE
    make_strip(sys.argv[1], sys.argv[2], size)
