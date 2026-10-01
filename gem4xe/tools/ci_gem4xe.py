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

QED GOES ON THE DISK AS QED.PRG, launched from gem4xe's own desktop
(DESKTOP.PRG -- the shell's name for it since gem4xe's e9213f3), and its
resource as QED.RSC; QED asks
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
# ...and gem4xe's own gate helpers, so the walk over the desktop's object
# tree is THE SAME CODE its gates use rather than a second copy that can
# drift: obj/children/placed/cstring/middle read the tree out of the
# target, and desk_g finds G whichever memory the desktop keeps it in.
sys.path.insert(0, os.path.join(GEM4XE, "tests", "emu"))

from a8test.launcher import launch            # noqa: E402
import symfile                                # noqa: E402
from shots import obj, children, placed, cstring, middle   # noqa: E402
from m17_desktop import desk_g                # noqa: E402
from deskref import g_offset, DROOT, WOBS_START   # noqa: E402
from aesref import W_FULLER                   # noqa: E402
from sdx816 import find_text                  # noqa: E402

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

# File > Save as..., the eighth item of the File drop-down, and the name
# list inside the selector it opens.
SAVEAS_Y = 70
LIST_X0, LIST_X1, LIST_Y0, LIST_Y1 = 200, 350, 96, 180
MIN_LIST_INK = 200      # six filenames of black text in the list box
# What SpartaDOS X has at $08C0 -- its own code, four bytes before the stub
# the DOS calls to read a DIRECTORY.  menu_text used to land a menu string
# on $0008C4 (gem4xe src/aes/menu.c, mn_text: the bank was dropped from a
# far tree's ob_spec, leaving the string's offset IN THE .RSC FILE, and
# QED's "  Makefile..." is at offset $08C4).  The DOS then jumped into a
# BRK on the next directory read and the machine died with the selector
# drawn -- which is why this is checked at the BYTES as well as at the
# screen: the picture alone cannot tell a drawn list from a frozen one.
SDX_STUB_AT = 0x08C0
SDX_STUB = bytes((0x0A, 0xBD, 0x02, 0x0A, 0xCD, 0x24, 0x0A, 0xF0, 0x03))
# Saving: the Selection field (its label runs to x~260, the field beyond),
# the OK button, and the name to write.
SEL_FIELD = (300, 80)
OK_BTN = (267, 201)
SAVE_NAME = "TEST.TXT"
# File > Open..., the second item, and File > Quit, the last; and the
# document the gate opens and drops.
OPEN_Y, QUIT_Y = 24, 133
DOC_NAME, DOC_FIRST = "DOC.TXT", "FIRST LINE"
KEYNAME = {".": "PERIOD", "_": "MINUS", " ": "SPACE"}
# The row the SEVENTH name lands on once the save has made one.  Measured,
# not guessed, and the first attempt was guessed and WRONG: it read 106
# black pixels on a row that was supposed to be empty, so it would have
# passed whether or not anything was saved.  With six files the name rows
# are y 109..155 and 156..162 is blank; the seventh name fills 157..163.
# Ink here is 2 px before the save and 148 after.
# DOC.TXT (tools/doc.txt, for the open and the drop below) is one more
# name before the save, so the new one lands on the EIGHTH row now.
LIST_ROW7 = (210, 350, 165, 171)
MIN_ROW7_INK = 40


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


def band_ink(path, x0, x1, y0, y1):
    """Near-black pixels in a rectangle -- the selector's name list."""
    try:
        from PIL import Image
    except ImportError:
        return None
    im = Image.open(path).convert("RGB")
    px = im.load()
    return sum(1 for y in range(y0, min(y1, im.height))
               for x in range(x0, min(x1, im.width))
               if sum(px[x, y]) < BLACK)


def main():
    problems = []
    for need in (DISK, G4A, RSC):
        if not os.path.isfile(need):
            print(f"FAIL: {need} is missing -- run `make check` (builds it) first")
            return 1
    desk_g4a = os.path.join(GEM4XE, "build", "desktop.g4a")
    want_len = os.path.getsize(desk_g4a)   # the SHELL reads the desktop
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

        # -- wait for THE DESKTOP to load ---------------------------------
        # It is the shell's own program, and the one the read above was of.
        # qed comes later, launched from it.
        near = 0
        while frames < 11000:
            near = b.peek16(sysm["app_near"])
            if near:
                break
            b.frames(200)
            frames += 200
        runs = b.peek16(sysm["sh_runs"])
        rc, ret = s16(b.peek16(sysm["sh_lastrc"])), s16(b.peek16(sysm["sh_lastret"]))
        print(f"  desktop   app_near=${near:04X}, sh_runs={runs}, "
              f"sh_lastrc={rc} ({ERR.get(rc, '?')}), sh_lastret={ret}")
        if not near:
            problems.append(
                f"app_near stayed 0 through {frames} frames, sh_lastrc {rc} "
                f"({ERR.get(rc, '?')}).  Before reading this as a load failure, "
                f"check {sym_path} belongs to the GEM.COM on the disk")
            return 1
        if rc not in (0, None):
            problems.append(f"sh_lastrc {rc} ({ERR.get(rc, '?')}), expected APP_OK")
        b.frames(900)                   # let the desk draw its icons

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

        def dclick(pos):
            """Two clicks inside the double-click time, the way a person
            opens an icon: the AES holds the first press for the whole
            delay before it decides, so the second has to land inside it."""
            move_to(pos)
            b.joy(JOY_PORT, "centre", fire=True); b.frames(2)
            b.joy(JOY_PORT, "centre", fire=False); b.frames(2)
            b.joy(JOY_PORT, "centre", fire=True); b.frames(2)
            b.joy(JOY_PORT, "centre", fire=False); b.frames(20)

        def click1(pos):
            move_to(pos)
            b.joy(JOY_PORT, "centre", fire=True); b.frames(14)
            b.joy(JOY_PORT, "centre", fire=False); b.frames(20)

        def gadget(which):
            """A gadget of the window on top, as W_ACTIVE was last laid
            out (gem4xe src/aes/wind.c)."""
            return middle(placed(b, sysm["W_ACTIVE"])[which])

        def screen_tree():
            """The desktop's g_screen, wherever its G lives -- far bss for a
            large-data build (gem4xe src/sys/app.c exports app_far)."""
            return desk_g(b, sysm) + g_offset("g_screen")

        def desk_icon(label):
            tree = screen_tree()
            rects = placed(b, tree)
            for i in children(b, tree, DROOT):
                if i < WOBS_START:
                    continue
                if cstring(b, obj(b, tree, i)["spec"] + 34) == label:
                    return middle(rects[i])
            raise KeyError(label)

        def win_item(name):
            """An entry of the window on top, by the start of its label."""
            tree = screen_tree()
            rects = placed(b, tree)
            seen = []
            for top in reversed(children(b, tree, 0)):
                if top == DROOT or not children(b, tree, top):
                    continue
                for i in children(b, tree, top):
                    o = obj(b, tree, i)
                    text = (cstring(b, o["spec"] + 34) if o["type"] & 0xFF == 31
                            else cstring(b, o["spec"], 48))
                    seen.append(text.strip())
                    if text.strip().startswith(name):
                        return middle(rects[i])
                break
            raise KeyError(f"{name} is not in the window on top: {seen}")

        # -- RUN QED FROM THE DESKTOP -------------------------------------
        # Not as the desktop.  This is the whole point of the arrangement:
        # the shell stays in place, and qed is one program it launched --
        # which is what anything later (a second application, an accessory
        # that outlives a program, multitasking) needs to be true.
        try:
            dclick(desk_icon("DISK D1:"))
            b.frames(400)
            # ...and GROW IT: the disk carries eight files and an unfulled
            # window shows four, so QED.PRG -- alphabetically after GEM.COM
            # -- is below the fold.  The fuller is what a person reaches
            # for, and it keeps this gate from depending on how many files
            # happen to fit.
            click1(gadget(W_FULLER))
            b.frames(300)
            dclick(win_item("QED.PRG"))
        except KeyError as e:
            print(f"FAIL: the desktop's window does not show it: {e}")
            return 1
        started, frames2 = 0, 0
        while frames2 < 9000:
            if b.peek16(sysm["sh_runs"]) >= 2:
                started = 1
                break
            b.frames(200)
            frames2 += 200
        near = b.peek16(sysm["app_near"])
        rc = s16(b.peek16(sysm["sh_lastrc"]))
        print(f"  launched  sh_runs={b.peek16(sysm['sh_runs'])} "
              f"(1 is the desktop, 2 is qed), app_near=${near:04X} "
              f"(linked ${link_near:04X}), sh_lastrc={rc} ({ERR.get(rc, '?')})")
        if not started:
            problems.append(
                f"sh_runs never reached 2 in {frames2} frames, sh_lastrc {rc} "
                f"({ERR.get(rc, '?')}) -- the desktop did not run QED.PRG.  "
                f"APP_E_POOL here means its near region no longer fits beside "
                f"the desktop (gem4xe tools/memreport.py says what is free)")
            return 1

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

        # -- and it SAVES: File > Save as... opens the selector -------------
        # Three assertions, because the first two are what a screenshot
        # cannot see.  The DOS's own bytes first: a menu string written
        # over them is the defect this step exists for, and it shows up
        # here BEFORE the selector is even opened.
        stub = bytes(b.memdump(SDX_STUB_AT, len(SDX_STUB)))
        print(f"  dos       SpartaDOS X at ${SDX_STUB_AT:04X}: {stub.hex(' ')}")
        if stub != SDX_STUB:
            problems.append(
                f"SpartaDOS X's code at ${SDX_STUB_AT:04X} reads {stub.hex(' ')}, "
                f"not {SDX_STUB.hex(' ')}: something has written over the DOS")

        move_to((FILE_X, BAR_Y))
        b.joy(JOY_PORT, "centre", fire=True)
        b.frames(14)
        move_to((FILE_X, SAVEAS_Y), speed=4)
        b.frames(4)
        b.joy(JOY_PORT, "centre", fire=False)
        b.frames(120)

        # Still RUNNING, and still native: the failure mode was the CPU
        # stopped in the OS's break dispatch in emulation mode, with the
        # selector's picture left on the screen looking perfectly healthy.
        r0 = b.cmd("REGS")
        b.frames(30)
        r1 = b.cmd("REGS")
        moved, native = r1["cycles"] != r0["cycles"], r1["E"] == 0
        print(f"  selector  after Save as: PC=${int(r1['PC'].lstrip('$'), 16):04X} "
              f"bank {r1['K']} E={r1['E']} running={moved}")
        if not (moved and native):
            problems.append(
                f"after File>Save as... the machine is "
                f"{'stopped' if not moved else 'in emulation mode'} at "
                f"{r1['K']}:{r1['PC']} -- the directory read wedged it")

        shot3 = os.path.join(os.path.dirname(SHOT), "qed-g4a-fsel.png")
        b.screenshot(shot3)
        ink = band_ink(shot3, LIST_X0, LIST_X1, LIST_Y0, LIST_Y1)
        if ink is not None:
            print(f"  listing   {ink} black pixels in the selector's name list "
                  f"({shot3})")
            if ink < MIN_LIST_INK:
                problems.append(
                    f"the selector's name list drew {ink} black pixels, under "
                    f"{MIN_LIST_INK}: the directory read answered nothing")

        # -- and it SAVES ---------------------------------------------------
        # Checked IN THE MACHINE: type a name, click OK, then reopen the
        # selector and require the file in its listing.  The host cannot
        # check the disk image here -- this run boots SpartaDOS X from a
        # CARTRIDGE, so the ATR is not the boot image and Altirra keeps
        # its writes out of the file -- and the listing is the better
        # question anyway: it is what the DOS believes, read back through
        # the same path a user would.
        def click(pos):
            move_to(pos)
            b.joy(JOY_PORT, "centre", fire=True); b.frames(8)
            b.joy(JOY_PORT, "centre", fire=False); b.frames(20)

        def pick(item_y):
            """The File menu, dragged to an item and released -- the same
            press-drag-release the two steps above do by hand."""
            move_to((FILE_X, BAR_Y))
            b.joy(JOY_PORT, "centre", fire=True); b.frames(14)
            move_to((FILE_X, item_y), speed=4); b.frames(4)
            b.joy(JOY_PORT, "centre", fire=False); b.frames(120)

        click(SEL_FIELD)
        for ch in SAVE_NAME:
            b.key(KEYNAME.get(ch, ch))
            b.frames(10)
        b.frames(20)
        click(OK_BTN)
        b.frames(300)
        shot4 = os.path.join(os.path.dirname(SHOT), "qed-g4a-saved.png")
        b.screenshot(shot4)
        title = black_ink(shot4, 42, 52)
        r = b.cmd("REGS")
        print(f"  saved     window title redrawn ({title} px); "
              f"PC={r['K']}:{r['PC']} E={r['E']}")
        if r["E"] != 0:
            problems.append("after OK the machine is in emulation mode: "
                            "the save wedged it")

        pick(SAVEAS_Y)                  # ...and ask the DOS for the listing
        shot5 = os.path.join(os.path.dirname(SHOT), "qed-g4a-listing.png")
        b.screenshot(shot5)
        ink7 = band_ink(shot5, *LIST_ROW7)
        print(f"  saved     {ink7} black pixels on the selector's seventh row, "
              f"where {SAVE_NAME} lands ({shot5})")
        if ink7 < MIN_ROW7_INK:
            problems.append(
                f"after saving {SAVE_NAME} the selector's listing gained no row "
                f"({ink7} px, under {MIN_ROW7_INK}): the file was not created")
        click((395, 201))               # Cancel, and leave it as we found it
        b.frames(60)

        # -- and it OPENS a document: File > Open..., the name, OK ---------
        # DOC.TXT is two CR LF lines (tools/doc.txt), the way an ST or a PC
        # writes a text file.  Opened through the same selector the save
        # used, and its first line must be on the screen.
        pick(OPEN_Y)
        click(SEL_FIELD)
        for _ in range(12):
            b.key("BACKSPACE"); b.frames(2)
        for ch in DOC_NAME:
            b.key(KEYNAME.get(ch, ch)); b.frames(8)
        click(OK_BTN)
        b.frames(600)
        shot6 = os.path.join(os.path.dirname(SHOT), "qed-g4a-open.png")
        b.screenshot(shot6)
        seen = find_text(shot6, DOC_FIRST)
        print(f"  opened    {DOC_NAME} through the selector: {DOC_FIRST!r} at {seen}")
        if seen is None:
            problems.append(f"File > Open... {DOC_NAME}: its first line is not on "
                            f"the screen ({shot6})")

        # -- and its MENU SHORTCUTS work: Ctrl-N, tapped ----------------------
        # cflib matches "^N" by the key's ST scan code and K_CTRL.  Before
        # gem4xe's phase 84 a letter came with no scan code and a tapped
        # key's CONTROL was gone by the time QED asked, so no shortcut ever
        # did anything.  A new window is a new "Untitled" title.
        b.key("N", ctrl=True)
        b.frames(300)
        shot_n = os.path.join(os.path.dirname(SHOT), "qed-g4a-ctrl-n.png")
        b.screenshot(shot_n)
        seen = find_text(shot_n, "Untitled")
        print(f"  shortcut  Ctrl-N: a new window's title at {seen}")
        if seen is None:
            problems.append(f"Ctrl-N opened no new window: the menu shortcuts do "
                            f"not reach QED ({shot_n})")

        # -- and a document DROPPED on QED.PRG opens with it ---------------
        # The desktop hands the file over as QED's command tail, and the
        # port makes argv of it (src/qed4xe.c, main): QED's own main takes
        # its files from argv.  Quit first, back to the desktop.
        runs = b.peek16(sysm["sh_runs"])
        pick(QUIT_Y)
        for _ in range(60):
            b.frames(100)
            if b.peek16(sysm["sh_runs"]) > runs:
                break
        b.frames(600)
        back = b.peek16(sysm["sh_runs"])
        print(f"  quit      File > Quit: the shell's runs {runs} -> {back} "
              f"(the desktop again)")
        if back <= runs:
            problems.append("File > Quit did not end QED: the desktop never came back "
                            "(a program's exit() must end it -- the kit's gemstub.c)")
            return 1
        try:
            dclick(desk_icon("DISK D1:")); b.frames(400)
            click1(gadget(W_FULLER)); b.frames(300)
            src, dst = win_item(DOC_NAME), win_item("QED.PRG")
        except KeyError as e:
            problems.append(f"back at the desktop, the window does not show it: {e}")
            src = dst = None
        if src:
            runs = b.peek16(sysm["sh_runs"])
            move_to(src)
            b.joy(JOY_PORT, "centre", fire=True); b.frames(14)
            move_to(dst, speed=4); b.frames(6)
            b.joy(JOY_PORT, "centre", fire=False)
            for _ in range(90):
                b.frames(100)
                if b.peek16(sysm["sh_runs"]) > runs:
                    break
            b.frames(1500)
            shot7 = os.path.join(os.path.dirname(SHOT), "qed-g4a-drop.png")
            b.screenshot(shot7)
            seen = find_text(shot7, DOC_FIRST)
            print(f"  dropped   {DOC_NAME} on QED.PRG: {DOC_FIRST!r} at {seen}")
            if seen is None:
                problems.append(f"{DOC_NAME} dropped on QED.PRG: QED started "
                                f"without it ({shot7})")

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
          "its menu from it, opens a document, takes typing, SAVES it and OPENS "
          "one through the file selector, QUITS to the desktop, and opens a "
          "document dropped on it, on an Atari 8-bit, 0 problem(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
