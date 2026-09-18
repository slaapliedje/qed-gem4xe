#!/usr/bin/env python3
"""Boot QED on an Atari 8-bit under gem4xe, and check what it drew.

QED is the first application somebody ELSE wrote to reach gem4xe: 37 C
files of a FreeMiNT GEM editor, plus cflib, ported against the published
kit alone (../PORT.md).  Linking it proved the surface is there; only a
run says the arrangement holds -- pointers are 24 bits, a far pointer's
arithmetic is 16, the image is 355 KB over six banks in the large data
model, and everything the AES reads in place is in bank $00.  None of
that shows to a compiler.

WHAT MAKES THIS GATE DIFFERENT from RetroWP's: QED does not build its
menu in code, it LOADS it.  qed.rsc is 33,014 bytes -- 682 objects, over
twice gem4xe's 14 KB application pool -- so rsrc_load must take the far
path (docs/far-trees.md, step 2): the resource goes to far memory and the
AES draws the menu bar from a tree it addresses with a 24-bit pointer.  A
drawn menu bar here is that path working for a real, third-party resource,
not a fixture.

WHY IT POLLS: gem4xe writes sh_desk_len only when a whole read finishes,
so a read in progress is indistinguishable from a failure at any single
sample.  A 355 KB read lands later than RetroWP's 202 KB, so the wait is
longer.  A timeout says how far the read got, not merely "failed".

QED GOES ON THE DISK AS DESKTOP.G4A and its resource as QED.RSC; QED asks
for "qed.rsc" through shel_find and SpartaDOS X, case-insensitive, finds
it.  The symbols MUST be the gem.sym beside the GEM.COM on the disk: they
are read by address, so a mismatched pair answers plausible nonsense
rather than failing (the lesson RetroWP's gate paid for).
"""
import math
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEM4XE = os.environ.get("GEM4XE", os.path.expanduser("~/dev/gem4xe"))

# The patched emulator, before the launcher is imported: /usr/bin/AltirraSDL
# lacks AltirraSDL PR #88 (the two 65C816 native-mode CPU fixes), and its
# IRQ storm corrupts bank $00, where the pool, the read buffer and the
# stack all live -- so an unpatched build measures the emulator, not QED.
os.environ.setdefault("ALTIRRASDL", os.path.expanduser("~/dev/altirra-patched/AltirraSDL"))
sys.path.insert(0, os.path.join(GEM4XE, "tools"))

from a8test.launcher import launch            # noqa: E402
import symfile                                # noqa: E402

DISK = os.path.join(ROOT, "build", "QED.ATR")
G4A = os.path.join(ROOT, "build", "QED.G4A")
RSC = os.path.join(ROOT, "build", "QED.RSC")
SHOT = os.path.join(ROOT, "build", "qed-g4a.png")

ERR = {0: "APP_OK", -1: "APP_E_MAGIC", -2: "APP_E_SHORT", -3: "APP_E_POOL",
       -4: "APP_E_FAR", -5: "APP_E_FIXUP", -6: "APP_E_FILE", -7: "APP_E_OLDSDK"}

# The screen is 640x240 and a screenshot doubles its rows, so the menu
# bar's ten rows are the top twenty of the picture.  The bar is drawn light
# with black titles; BLACK ink there (not merely "not white") is QED's menu
# text -- qed, File, Edit, Window, Search, Special, Options, Shell -- drawn
# from the tree the far resource load produced.  That is the assertion.
BAR_ROWS = 20
MIN_BAR_INK = 400       # eight titles of black text across the bar
BLACK = 96              # sum(RGB) under this is ink, not the light bar or the
                        # coloured desktop -- both of which are far from black
# Below the bar QED shows its desktop and, once a document is opened, an
# edit window.  At start-up it is the empty desktop.
BODY_TOP, BODY_BOT = 22, 220

# -- driving the editor ------------------------------------------------------
#
# The File title, measured off the menu bar; and the first item of its
# drop-down, "New text".
FILE_X, BAR_Y, ITEM_Y = 79, 5, 17
# gem4xe's pointer device sits on joystick port 2, which is bridge joy
# index 1.  THE BUTTON CANNOT BE POKED into ptr_state: position is
# relative and a poke sticks, but the left button is the port's trigger,
# read absolutely every poll, so a poked press is cleared on the next
# frame (measured).  It goes through the emulated trigger instead.
JOY_PORT = 1
# The document's first text row, just under the window's info line.
TEXT_TOP, TEXT_BOT = 60, 72
TYPED = "QED"
MIN_TEXT_INK = 40       # three 8x8 glyphs of black text


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def link_near_of(path):
    with open(path, "rb") as f:
        return struct.unpack("<H", f.read(8)[4:6])[0]


def black_ink(path, y0, y1):
    """Near-black pixels in a band -- text, not the light menu bar or the
    coloured desktop, both of which are nowhere near black."""
    try:
        from PIL import Image
    except ImportError:
        return None
    im = Image.open(path).convert("RGB")
    px = im.load()
    return sum(1 for y in range(y0, min(y1, im.height)) for x in range(im.width)
               if sum(px[x, y]) < BLACK)


def main():
    problems = []
    for need in (DISK, G4A, RSC):
        if not os.path.isfile(need):
            print(f"FAIL: {need} is missing -- run `make check` (builds it) first")
            return 1
    want_len = os.path.getsize(G4A)
    link_near = link_near_of(G4A)
    rsc_len = os.path.getsize(RSC)

    sym_path = os.environ.get("GEM4XE_SYM") or os.path.join(GEM4XE, "build", "gem.sym")
    sysm = symfile.load(sym_path)
    print(f"  symbols   {sym_path}")
    print(f"  resource  qed.rsc is {rsc_len} bytes -- the pool is 14336, so "
          f"rsrc_load must take the far path")
    cart = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("SDX_CART")
    if not cart:
        print("FAIL: no SpartaDOS X cartridge: pass one, or set SDX_CART")
        return 1

    emu = launch(tag="qed", memsize="1088K",
                 extra_args=["--disk", DISK, "--cart", cart])
    b = emu.bridge
    try:
        u32 = lambda a: int.from_bytes(bytes(b.memdump(a, 4)), "little")  # noqa: E731

        # -- wait for the whole image to be read --------------------------
        frames, ln = 0, 0
        while frames < 8000:
            b.frames(200)
            frames += 200
            ln = u32(sysm["sh_desk_len"])
            if ln:
                break
        if not ln:
            print(f"FAIL: no read finished in {frames} frames.  A 355 KB read "
                  f"lands later than RetroWP's 202 KB, but this is still a real "
                  f"failure -- check the emulator is the patched one first")
            return 1
        print(f"  read      sh_desk_len={ln} after ~{frames} frames")
        if ln != want_len:
            problems.append(f"read {ln} bytes, the file is {want_len}")

        # -- wait for the program to load ---------------------------------
        near = 0
        while frames < 11000:
            near = b.peek16(sysm["app_near"])
            if near:
                break
            b.frames(200)
            frames += 200
        runs = b.peek16(sysm["sh_runs"])
        rc, ret = s16(b.peek16(sysm["sh_lastrc"])), s16(b.peek16(sysm["sh_lastret"]))
        print(f"  load      app_near=${near:04X} (linked ${link_near:04X}), "
              f"sh_runs={runs}, sh_lastrc={rc} ({ERR.get(rc, '?')}), "
              f"sh_lastret={ret}")
        if not near:
            problems.append(
                f"app_near stayed 0 through {frames} frames, sh_lastrc {rc} "
                f"({ERR.get(rc, '?')}).  Before reading this as a load failure, "
                f"check {sym_path} belongs to the GEM.COM on the disk")
        if rc not in (0, None):
            problems.append(f"sh_lastrc {rc} ({ERR.get(rc, '?')}), expected APP_OK")

        # -- QED runs its own start-up: appl_init, rsrc_load (far), the
        #    menu bar, an empty edit window -- give it time, then look ----
        if near:
            b.frames(1200)
        b.screenshot(SHOT)
        bar = black_ink(SHOT, 0, BAR_ROWS)
        body = black_ink(SHOT, BODY_TOP, BODY_BOT)
        if bar is None:
            print(f"  screen    {SHOT} (no PIL here, so not measured)")
        else:
            print(f"  menu bar  {bar} black pixels of title text ({SHOT})")
            print(f"  desktop   {body} black pixels below the bar (empty at start-up)")
            if bar < MIN_BAR_INK:
                problems.append(f"the menu bar drew {bar} pixels of black text, "
                                f"under {MIN_BAR_INK}: qed.rsc did not load far, "
                                f"or its menu did not draw")

        # -- and it EDITS: File > New text, then type ----------------------
        # The menu is opened by a PRESS, not a hover: gem4xe's menu manager
        # waits on MU_BUTTON | MU_M1 and QED asks for MU_M1 only when
        # mouse_sleeps(), so moving onto the bar drops nothing.  Classic
        # GEM from there: drag down the drop-down, release over the item.
        ptr = sysm["ptr_state"]

        def at():
            return b.peek16(ptr), b.peek16(ptr + 2)

        def poke16(a, v):
            b.poke(a, v & 0xFF)
            b.poke(a + 1, (v >> 8) & 0xFF)

        def move_to(dst, speed=10):
            """Interpolated, one step a frame, as m17 moves the pointer: the
            AES acts on the pointer ENTERING a rectangle, and a teleport
            does not read as an entry."""
            src = at()
            dx, dy = dst[0] - src[0], dst[1] - src[1]
            n = max(2, math.ceil(math.hypot(dx, dy) / speed * 1.5))
            for i in range(1, n + 1):
                t = i / n
                t = t * t * (3 - 2 * t)
                poke16(ptr, round(src[0] + dx * t))
                poke16(ptr + 2, round(src[1] + dy * t))
                b.frames(1)

        move_to((FILE_X, BAR_Y))
        b.joy(JOY_PORT, "centre", fire=True)
        b.frames(14)                    # through the double-click delay
        move_to((FILE_X, ITEM_Y), speed=4)
        b.frames(4)
        b.joy(JOY_PORT, "centre", fire=False)
        b.frames(60)
        move_to((300, 120))             # out of the bar: while the control
        b.frames(10)                    # manager owns the mouse it eats keys
        for ch in TYPED:
            b.key(ch)
            b.frames(10)
        b.frames(40)
        shot2 = os.path.join(os.path.dirname(SHOT), "qed-g4a-edit.png")
        b.screenshot(shot2)
        ink = black_ink(shot2, TEXT_TOP, TEXT_BOT)
        if ink is not None:
            print(f"  editing   File>New, typed {TYPED!r}: {ink} black pixels on "
                  f"the document's first line ({shot2})")
            if ink < MIN_TEXT_INK:
                problems.append(
                    f"the document's first line drew {ink} black pixels, under "
                    f"{MIN_TEXT_INK}: File>New did not open a window, or the "
                    f"typed text was not inserted and painted")
    finally:
        try:
            b.close()
        except Exception:
            pass
        try:
            emu.proc.kill()
        except Exception:
            pass

    if problems:
        for p in problems:
            print(f"FAIL: {p}")
        return 1
    print("qed-gem4xe: PASS -- QED loads a 33 KB resource into far memory, draws "
          "its menu from it, opens a document and takes typing, on an Atari "
          "8-bit, 0 problem(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
