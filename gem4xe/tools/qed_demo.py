#!/usr/bin/env python3
"""Record QED being used on gem4xe: build/movie/qed.mp4 and .gif.

A session on the emulated machine, one frame at a time -- the same shape
as gem4xe's own tests/emu/demo_aes.py, which this borrows its Camera and
its ffmpeg recipe from.  Nothing here is staged: every pixel is QED
running on an emulated Atari XL/XE with VBXE and a Rapidus 65C816, driven
at the mouse and the keyboard exactly as the gate drives it.

What the session shows, in order:

  the desktop and QED's menu bar, drawn from a 33 KB resource that does
  not fit the 14 KB application pool and therefore lives in far memory;
  the File menu pulled down; New text opening a document window; a few
  lines typed into it; then File > Save as..., the AES file selector, a
  name typed, and OK -- after which the title bar's '*' is gone.

THE BOOT IS NOT RECORDED.  Reading a 355 KB program off a floppy takes
about 4,200 frames, or eighty-odd seconds of nothing moving; the camera
is installed once QED is up.

    python3 tools/qed_demo.py [--no-encode] [--keep-frames]
"""
import math
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEM4XE = os.environ.get("GEM4XE", os.path.expanduser("~/dev/gem4xe"))
os.environ.setdefault("ALTIRRASDL", os.path.expanduser("~/dev/altirra-patched/AltirraSDL"))
sys.path.insert(0, os.path.join(GEM4XE, "tools"))

from a8test.launcher import launch          # noqa: E402
import symfile                              # noqa: E402
import vbxeref                              # noqa: E402

DISK = os.path.join(ROOT, "build", "QED.ATR")
MOVIEDIR = os.path.join(ROOT, "build", "movie")
FRAMEDIR = os.path.join(MOVIEDIR, "frames")

# Measured off the running program (the menu bar's titles, and the File
# drop-down's 8-pixel item cells starting at y=11 with separator lines at
# 54, 94 and 118).
FILE_X, BAR_Y = 79, 5
NEW_TEXT_Y = 14
SAVE_AS_Y = 70
# The AES file selector, measured the same way.  Mind the two underscore
# rows: the DIRECTORY line is y=71 and spans x 216..470, the SELECTION
# field is y=83 and spans x 256..350.  Clicking the first one types the
# name into the path and OK then has nothing to save.
FSEL_NAME = (280, 83)           # the Selection field
FSEL_OK = (251, 200)
JOY_PORT = 1                    # gem4xe's pointer is on joystick port 2

# Typed as three lines.  Capitals need the shift row of the OS key table,
# which is what `shift` does here; SPACE and the digits are plain keys.
LINES = [
    [("Q", 1), ("E", 1), ("D", 1), ("SPACE", 0), ("O", 1), ("N", 1),
     ("SPACE", 0), ("G", 1), ("E", 1), ("M", 1), ("4", 0), ("X", 1), ("E", 1)],
    [("G", 1), ("E", 1), ("M", 1), ("SPACE", 0), ("F", 1), ("O", 1), ("R", 1),
     ("SPACE", 0), ("T", 1), ("H", 1), ("E", 1), ("SPACE", 0),
     ("A", 1), ("T", 1), ("A", 1), ("R", 1), ("I", 1), ("SPACE", 0),
     ("8", 0), ("SPACE", 0), ("B", 1), ("I", 1), ("T", 1)],
    [("6", 0), ("5", 0), ("C", 1), ("8", 0), ("1", 0), ("6", 0), ("SPACE", 0),
     ("A", 1), ("N", 1), ("D", 1), ("SPACE", 0), ("V", 1), ("B", 1), ("X", 1),
     ("E", 1)],
]
SAVENAME = [("D", 1), ("E", 1), ("M", 1), ("O", 1)]


class Camera:
    """Every frame the session runs, screenshotted: b.frames becomes one
    FRAME and one SCREENSHOT per frame (gem4xe's demo_aes.py)."""

    def __init__(self, b, outdir):
        self.b, self.dir, self.n = b, outdir, 0
        self.inner = b.frames

    def frames(self, n):
        r = None
        for _ in range(n):
            r = self.inner(1)
            self.b.screenshot(os.path.join(self.dir, f"f{self.n:05d}.png"))
            self.n += 1
        return r


def encode(framedir, out_base):
    """The overlay's 640x240 out of the 672-wide shot, scaled with nearest
    neighbour so the pixels stay square-edged; mp4 at the rate the machine
    ran at, gif at half."""
    src = os.path.join(framedir, "f%05d.png")
    crop = f"crop={vbxeref.SHOT_W}:{vbxeref.SHOT_H}:{vbxeref.SHOT_X0}:{vbxeref.SHOT_Y0}"
    mp4, gif = out_base + ".mp4", out_base + ".gif"
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "50",
                    "-i", src, "-vf",
                    f"{crop},scale=1280:960:flags=neighbor,format=yuv420p",
                    "-c:v", "libx264", "-crf", "18", "-preset", "slow", mp4],
                   check=True)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "50",
                    "-i", src, "-vf",
                    f"fps=25,{crop},scale=640:480:flags=neighbor,"
                    "split[a][b];[a]palettegen=max_colors=32[p];[b][p]paletteuse=dither=none",
                    gif], check=True)
    return mp4, gif


def main(argv):
    do_encode = "--no-encode" not in argv
    keep = "--keep-frames" in argv
    cart = os.environ.get("SDX_CART") or next(
        (a for a in argv if a.endswith(".car") or a.endswith(".rom")), None)
    if not cart:
        import tomllib
        with open(os.path.join(GEM4XE, "fixtures.toml"), "rb") as f:
            cart = tomllib.load(f)["spartados"]["sdx_cart"]
    if not os.path.isfile(DISK):
        print(f"FAIL: {DISK} is missing -- run `make build/QED.ATR` first")
        return 1

    shutil.rmtree(FRAMEDIR, ignore_errors=True)
    os.makedirs(FRAMEDIR, exist_ok=True)
    sysm = symfile.load(os.path.join(GEM4XE, "build", "gem.sym"))
    emu = launch(tag="qeddemo", memsize="1088K",
                 extra_args=["--disk", DISK, "--cart", cart])
    b = emu.bridge
    ptr = sysm["ptr_state"]
    try:
        u32 = lambda a: int.from_bytes(bytes(b.memdump(a, 4)), "little")  # noqa: E731

        # -- the boot, at speed and off camera -------------------------
        frames = 0
        while frames < 8000:
            b.frames(200)
            frames += 200
            if u32(sysm["sh_desk_len"]):
                break
        while frames < 11000 and not b.peek16(sysm["app_near"]):
            b.frames(200)
            frames += 200
        b.frames(1200)
        print(f"QED up after ~{frames} frames; rolling")

        cam = Camera(b, FRAMEDIR)
        b.frames = cam.frames           # from here every frame is a picture

        def at():
            return b.peek16(ptr), b.peek16(ptr + 2)

        def poke16(a, v):
            b.poke(a, v & 0xFF)
            b.poke(a + 1, (v >> 8) & 0xFF)

        def move_to(dst, speed=8):
            src = at()
            dx, dy = dst[0] - src[0], dst[1] - src[1]
            n = max(2, math.ceil(math.hypot(dx, dy) / speed * 1.5))
            for i in range(1, n + 1):
                t = i / n
                t = t * t * (3 - 2 * t)
                poke16(ptr, round(src[0] + dx * t))
                poke16(ptr + 2, round(src[1] + dy * t))
                b.frames(1)

        def press(down):
            b.joy(JOY_PORT, "centre", fire=down)
            b.frames(2)

        def click(xy):
            move_to(xy)
            press(True)
            b.frames(16)
            press(False)
            b.frames(12)

        def menu_pick(y, dwell=34):
            """Press on File, let the drop-down be seen, drag to the item
            and release -- the gesture a person makes."""
            move_to((FILE_X, BAR_Y))
            press(True)
            b.frames(dwell)
            move_to((FILE_X, y), speed=5)
            b.frames(8)
            press(False)

        def typ(keys, gap=7):
            for name, sh in keys:
                b.key(name, shift=bool(sh))
                b.frames(gap)

        # -- the session ------------------------------------------------
        b.frames(45)                            # the bar, drawn from far memory
        menu_pick(NEW_TEXT_Y)
        b.frames(55)                            # the window opens
        move_to((300, 120))                     # out of the bar: while the
        b.frames(14)                            # control manager owns the
                                                # mouse it eats the keys
        for i, line in enumerate(LINES):
            typ(line)
            if i != len(LINES) - 1:
                b.key("RETURN")
                b.frames(10)
        b.frames(70)

        # NO SAVE STEP.  File > Save as... opens the AES file selector and
        # it comes up WEDGED: its file list is empty on a disk that has
        # files, the pointer stops being tracked, and neither a click on
        # Cancel nor any typing reaches it.  That is gem4xe's selector (or
        # the path QED hands it), not the editor, and it is being chased
        # separately -- a demo is not the place to show it.  So the session
        # ends on the document, which is the thing that works.
        move_to((430, 150))
        b.frames(70)
        print(f"recorded {cam.n} frames")
    finally:
        try:
            b.close()
        except Exception:
            pass
        try:
            emu.proc.kill()
        except Exception:
            pass

    if do_encode:
        mp4, gif = encode(FRAMEDIR, os.path.join(MOVIEDIR, "qed"))
        print(f"  {mp4}\n  {gif}")
    if not keep:
        shutil.rmtree(FRAMEDIR, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
