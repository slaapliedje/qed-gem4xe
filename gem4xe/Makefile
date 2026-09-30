# QED on gem4xe -- the build.
#
#     make            build/QED.G4A, the editor for an Atari XL/XE running gem4xe
#     make check      boots it under AltirraSDL and looks (tools/ci_gem4xe.py)
#     make clean
#
# What is built, and from where:
#
#   ../src/*.c           QED itself, all 37 files, unmodified, --char-is-unsigned
#                        as its own Makefile builds it (-funsigned-char).
#   $(CFLIB_SRC)/*.c     cflib, the ST application library QED links
#                        (-lcflib), straight from the freemint/cflib checkout
#                        cflib4xe keeps, unmodified, minus the aggregate
#                        files that duplicate the one-function files and the
#                        four this machine cannot use.  Archived, so only
#                        what QED references is linked.
#   cflib/               cflib4xe's three-file compatibility shim for cflib,
#                        with its kit include pointed at THIS build's kit.
#   include/             what QED's #includes of MiNT headers resolve to here,
#                        and the port's one door into every translation unit:
#                        include/gem.h is the kit's gem.h plus qed4xe.h.
#   src/                 the port's own code: what QED and cflib call that the
#                        kit does not serve, answered truthfully for this
#                        machine (src/qed4xe.c), and a heap (src/qedmem.c).
#   $(GEM4XE_SDK)        the gem4xe application kit, copied to build/kit once
#                        per kit so a suite relinking gem4xe's build/ cannot
#                        change the headers under a compile.
#
# The three numbers below are the near budget the link enforces; the far
# image gets CBANKS banks of code and FBANKS of variables.  Start from
# RetroWP's, which is 202 KB over four banks and runs; the map says what
# QED really needs.

GEM4XE_SDK ?= $(HOME)/dev/gem4xe/build/gem4xe-sdk
GEM4XE     ?= $(HOME)/dev/gem4xe
CFLIB_SRC  ?= $(HOME)/dev/cflib4xe/upstream/cflib
CALYPSI    ?= $(HOME)/dev/toolchains/calypsi-65816

# WHAT THE MAP SAYS IT USES, not round numbers -- because the near region
# is a page-aligned slice of gem4xe's 14 KB application pool, and asking
# for more than you use is what decided whether qed could be LAUNCHED from
# the desktop or had to replace it.
#
# It asked 8192/3072 and the map answered 62.8% of the bss (5,145 bytes,
# of which 4,096 is the stack and 1,045 the three QED_NEAR objects) and
# 1.0% of the bits (31 bytes).  256 + 5632 + 256 is 6,144 -- twenty-four
# pages exactly -- against a pool that has about 9.4 KB free with the
# desktop and two accessories resident (gem4xe tools/memreport.py).
#
# Replacing the desktop was never a good arrangement: a machine that may
# one day multitask wants its shell left in place.
BSS    ?= 5632
BITS   ?= 256
STACK  ?= 4096
CBANKS ?= 6
FBANKS ?= 1

CC = $(CALYPSI)/bin/cc65816
AS = $(CALYPSI)/bin/as65816
LD = $(CALYPSI)/bin/ln65816
AR = $(CALYPSI)/bin/nlib

B   = build
KIT = $(B)/kit

MODEL = --code-model=large --data-model=large -O2
# cflib.h defines _WORD/_UWORD only under __GNUC__ or __PUREC__, and uses
# them before any header of ours is reached, so they come from the command
# line (cflib4xe FINDINGS.md 7).  QED uses _WORD too.
DEFS  = -D_WORD=short -D"_UWORD=unsigned short"
# include/ first: its gem.h is the door.  Everything else this port
# used to keep there -- osbind.h, mintbind.h, gemx.h, macros.h, support.h,
# strings.h, dirent.h -- is the KIT's now, which is the whole point of
# having written them twice: the next port does not.
#
# cflib/ IS BEFORE THE KIT, and that ordering is load-bearing.  cflib's
# intern.h takes <tos.h> when the compiler is not GCC, so cflib/tos.h is
# standing in for the ST's system header and carries cflib's own
# CF_GEMPARBLK with it.  The kit ships a <tos.h> too (it is one of the two
# names an ST source uses), and with the kit first the kit's wins and
# cflib stops compiling.  A port's own shims shadow the kit's defaults --
# which is why include/ has always been first, and cflib/ belongs beside
# it rather than after.  The kit's directory is still on
# the path for the headers the kit ships beside gem.h (sys/stat.h,
# mint/cookie.h) and cflib's for <cflib.h>.
INC   = -I include -I cflib -I $(KIT)/include -I $(CFLIB_SRC)

QED_CFLAGS   = $(MODEL) --char-is-unsigned $(DEFS) -DQED_BANKED_MALLOC $(INC) -I ../src
CFLIB_CFLAGS = $(MODEL) $(DEFS) $(INC)
PORT_CFLAGS  = $(MODEL) $(DEFS) $(INC)
KIT_CFLAGS   = $(MODEL) -I $(KIT)/include

# -- sources -----------------------------------------------------------------

QED_SRCS = $(wildcard ../src/*.c)
QED_OBJS = $(patsubst ../src/%.c,$(B)/qed/%.o,$(QED_SRCS))

# cflib ships two parallel organisations -- 27 aggregate files for Pure C
# and one-function files for GNU make -- and five aggregates collide with
# the granular set (cflib4xe FINDINGS.md 25).  Then the files this machine
# cannot use or does better itself, each with its replacement in
# src/qed4xe.c named:
#   filesel.c   gem4xe serves fsel_exinput; select_file is ours
#   fontsel.c   one system font; do_fontsel is a dead-print stub
#   userdef.c   G_USERDEF draws nothing here, so fix_dial/fix_menu/fix_popup
#               are no-ops that leave dialogs in plain AES-drawn form -- the
#               right rendering for this machine, not a loss
#   alerts.c    do_alert/do_walert route to the AES's own form_alert
#   asciitab.c  the insert-by-code dialog needs the inline-resource system;
#   asciiset.c  ascii_table answers -1 (absent) and the editor is whole
#   purec.c     Pure C only (and would duplicate objc_sysvar/objc_change_grect)
#   dirent.c    MiNT's Dopendir; ours is one Fsfirst
# The five aggregates above are cflib's Pure C bundles whose one-function
# granular twins are also built (cflib.prj lists 27 such bundles); building
# both duplicates symbols.  comm/form_do/misc/popup/scrap are the ones QED
# pulls -- found by llvm-nm over the compiled objects, not guessed -- and
# dropping them leaves zero residual duplicates.  file/magx/objc/vdi/nkcc
# are the other bundles, dropped as aggregates too (nkcc: NKCC is used
# through its granular files, which supply gem_to_norm/nkc_init/nkc_toupper).
# NKCC keyboard normaliser: gem4xe delivers a Ctrl-letter as the control
# character with scan 0, which cflib's nkc_tos2n would pass through as the
# bare code rather than QED's NKF_CTRL form.  So gem_to_norm and nkc_init
# are ours (src/qed4xe.c), converting gem4xe's key codes directly, and the
# two files that define those symbols -- nkccgton.c and nkccinit.c -- are
# dropped.  nkc_toupper (nkccuppr.c) and the rest are standalone and kept.
CFLIB_DROP = file.c magx.c objc.c vdi.c nkcc.c \
             comm.c form_do.c misc.c popup.c scrap.c \
             nkccgton.c nkccinit.c nkccntog.c \
             filesel.c fontsel.c userdef.c purec.c dirent.c \
             alerts.c asciitab.c asciiset.c
CFLIB_SRCS = $(filter-out $(addprefix $(CFLIB_SRC)/,$(CFLIB_DROP)),\
                          $(wildcard $(CFLIB_SRC)/*.c))
CFLIB_OBJS = $(patsubst $(CFLIB_SRC)/%.c,$(B)/cflib/%.o,$(CFLIB_SRCS))

PORT_OBJS = $(B)/port/qed4xe.o $(B)/port/qedmem.o $(B)/port/qedstub.o $(B)/port/cflibc.o

# The kit's objects, less lib/clib.c: that file is the kit's str*
# functions and its refusal of malloc.  QED needs a malloc, so Calypsi's
# str* are linked instead, and QED has a heap of its own (src/qedmem.c).
# The kit is LGPL-2.1-or-later (its COPYING.LIB), which is what lets QED
# keep its own terms.
# gemtime.o carries the kit's <time.h>: a 32-bit time_t, so that nothing
# here reaches Calypsi's _Div64, which falls through into whatever the
# linker placed after it (gem4xe tools/ccbug, B20).  That is what killed
# Save as... -- qed asked for the time, the C library's localtime()
# divided a 64-bit time_t, and the machine took a BRK inside a string.
KIT_OBJS = $(KIT)/crt_gemapp.o $(KIT)/gemabi.o $(KIT)/gemlib.o \
           $(KIT)/gemstat.o $(KIT)/gemtime.o $(KIT)/gemcompat.o \
           $(KIT)/gemstub.o

# -- the kit, snapshotted ----------------------------------------------------

$(KIT)/include/gem.h: $(GEM4XE_SDK)/include/gem.h $(GEM4XE_SDK)/lib/gemlib.c
	rm -rf $(KIT)
	mkdir -p $(B)
	cp -r $(GEM4XE_SDK) $(KIT)
	touch $@

$(KIT)/%.o: $(KIT)/lib/%.s | $(KIT)/include/gem.h
	$(AS) -o $@ $<
$(KIT)/%.o: $(KIT)/lib/%.c | $(KIT)/include/gem.h
	$(CC) $(KIT_CFLAGS) -o $@ $<
# As the kit's own Makefile builds these two: without the flag, -O2 shares
# one tail across the bindings and a large-data program links bindings it
# never calls.
$(KIT)/gemlib.o: $(KIT)/lib/gemlib.c | $(KIT)/include/gem.h
	$(CC) $(KIT_CFLAGS) --no-interprocedural-cross-jump -o $@ $<
$(KIT)/gemstub.o: $(KIT)/lib/gemstub.c | $(KIT)/include/gem.h
	$(CC) $(KIT_CFLAGS) --no-interprocedural-cross-jump -o $@ $<

# -- objects -----------------------------------------------------------------

HDRS = include/gem.h include/qed4xe.h $(KIT)/include/gem.h

$(B)/qed $(B)/cflib $(B)/port:
	mkdir -p $@

$(B)/qed/%.o: ../src/%.c $(HDRS) | $(B)/qed
	$(CC) $(QED_CFLAGS) -o $@ $<

# QED's main() is QED's, unchanged -- but it takes the file to open from
# argv, and a gem4xe program is started with argc 0: the command tail is
# read with shel_read, as the kit's README says.  So QED's main is
# compiled as qed_main, and the port's main (src/qed4xe.c) makes argv out
# of the tail and calls it.  Without this a file dropped on QED.PRG
# started QED and opened nothing.
$(B)/qed/main.o: ../src/main.c $(HDRS) | $(B)/qed
	$(CC) $(QED_CFLAGS) -Dmain=qed_main -o $@ $<

$(B)/cflib/%.o: $(CFLIB_SRC)/%.c $(HDRS) cflib/tos.h | $(B)/cflib
	$(CC) $(CFLIB_CFLAGS) -o $@ $<

$(B)/port/%.o: src/%.c $(HDRS) | $(B)/port
	$(CC) $(PORT_CFLAGS) -o $@ $<
$(B)/port/cflibc.o: cflib/cflibc.c $(HDRS) cflib/tos.h | $(B)/port
	$(CC) $(PORT_CFLAGS) -o $@ $<

# The archive: nlib rewrites its first argument from the rest.  NEVER name
# a library under $(CALYPSI) here -- a bare archive name is rewritten.
$(B)/libcflib.a: $(CFLIB_OBJS)
	rm -f $@
	$(AR) $@ $(CFLIB_OBJS)

# -- the link, three times, and the .G4A ------------------------------------

LDOPT = $(KIT)/lib/gemapp.scm $(KIT_OBJS) $(QED_OBJS) $(PORT_OBJS) \
        $(B)/libcflib.a clib-lc-ld.a \
        --rtattr exit=simplified --cstartup gemapp --stack-size $(STACK)
LDDEPS = $(KIT_OBJS) $(QED_OBJS) $(PORT_OBJS) $(B)/libcflib.a $(KIT)/lib/gemapp.scm

$(B)/qed.elf: $(LDDEPS)
	$(LD) $(LDOPT) -o $@ --list-file $(B)/qed.map \
	    --memories-expression "(app-layout-n #x1000 #x020000 $(BSS) $(BITS) $(CBANKS) $(FBANKS))"
$(B)/qed-near.elf: $(LDDEPS)
	$(LD) $(LDOPT) -o $@ \
	    --memories-expression "(app-layout-n #x1100 #x020000 $(BSS) $(BITS) $(CBANKS) $(FBANKS))"
$(B)/qed-far.elf: $(LDDEPS)
	$(LD) $(LDOPT) -o $@ \
	    --memories-expression "(app-layout-n #x1000 #x030000 $(BSS) $(BITS) $(CBANKS) $(FBANKS))"

$(B)/QED.G4A: $(B)/qed.elf $(B)/qed-near.elf $(B)/qed-far.elf
	python3 $(KIT)/tools/mkg4a.py $(B)/qed.elf $(B)/qed-near.elf \
	    $(B)/qed-far.elf $@ --syms $(B)/qed.sym

# The English resource, under the name QED asks rsrc_load for.
$(B)/QED.RSC: ../en/qed_en.rsc
	cp $< $@

all: $(B)/QED.G4A $(B)/QED.RSC

# -- the boot disk and the gate ---------------------------------------------
#
# QED goes on a SpartaDOS floppy as an APPLICATION the desktop launches,
# beside its own resource as QED.RSC, which QED asks for as "qed.rsc"
# through shel_find (SDX is case-insensitive).
#
# IT USED TO BE DESKTOP.G4A -- gem4xe's shell reads that name off the boot
# disk and runs it -- because it could not fit beside the desktop: it asked
# 11,520 bytes of a 14,336-byte pool that already held the desktop and its
# resource.  Two changes on 2026-09-20 ended that: gem4xe's rs_load now
# puts a resource in far memory for any caller that can hold a 32-bit
# pointer (so DESKTOP.RSC left bank $00), and QED's own near request came
# down to the 6,144 its map supports.  A program that IS the shell can
# never be one of several, so this matters for anything later -- a second
# application, an accessory outliving a program, multitasking.
#
# The disk carries gem4xe's GEM.COM (its build/gem.xex), its desktop and
# the resources the desktop wants, and nothing of the DOS: SpartaDOS X
# comes from the cartridge.  The base floppy and the cartridge are the two
# fixtures gem4xe names in its fixtures.toml.

GEM4XE_XEX  ?= $(GEM4XE)/build/gem.xex
GEM4XE_SYM  ?= $(GEM4XE)/build/gem.sym
GEM4XE_FIX  := $(shell python3 -c "import tomllib;print(tomllib.load(open('$(GEM4XE)/fixtures.toml','rb'))['spartados']['disk_32'])" 2>/dev/null)
GEM4XE_CART := $(shell python3 -c "import tomllib;print(tomllib.load(open('$(GEM4XE)/fixtures.toml','rb'))['spartados']['sdx_cart'])" 2>/dev/null)

GEM4XE_DESK := $(GEM4XE)/build/desktop.g4a
$(B)/QED.ATR: $(B)/QED.G4A $(B)/QED.RSC $(GEM4XE_XEX) $(GEM4XE_DESK) tools/doc.txt
	@test -n "$(GEM4XE_FIX)" || { echo "no SpartaDOS floppy: set [spartados].disk_32 in $(GEM4XE)/fixtures.toml"; exit 1; }
	python3 $(GEM4XE)/tools/mkspdisk.py "$(GEM4XE_FIX)" $(GEM4XE_XEX) $@ \
	    --sectors 8192 --name GEM.COM --boot GEM \
	    --add $(GEM4XE_DESK) DESKTOP.PRG \
	    --add $(GEM4XE)/build/desktop.rsc DESKTOP.RSC \
	    --add $(GEM4XE)/build/prefs.rsc PREFS.RSC \
	    --add $(GEM4XE)/build/lang.rsc LANG.RSC \
	    --add $(B)/QED.G4A QED.PRG \
	    --add $(B)/QED.RSC QED.RSC \
	    --add tools/doc.txt DOC.TXT

# A recording of QED being used, for showing people: build/movie/qed.mp4
# and .gif.  Every frame is the emulated machine, driven the way the gate
# drives it; the boot is deliberately not in shot.
demo: $(B)/QED.ATR
	@test -n "$(GEM4XE_CART)" || { echo "no SpartaDOS X cartridge: set [spartados].sdx_cart in $(GEM4XE)/fixtures.toml"; exit 1; }
	@GEM4XE="$(GEM4XE)" SDX_CART="$(GEM4XE_CART)" python3 tools/qed_demo.py

check: $(B)/QED.ATR
	@test -n "$(GEM4XE_CART)" || { echo "no SpartaDOS X cartridge: set [spartados].sdx_cart in $(GEM4XE)/fixtures.toml"; exit 1; }
	@GEM4XE="$(GEM4XE)" GEM4XE_SYM="$(GEM4XE_SYM)" python3 tools/ci_gem4xe.py "$(GEM4XE_CART)"

# Compile everything and report what did not, without stopping at the first.
compile: $(KIT)/include/gem.h
	@$(MAKE) -k $(QED_OBJS) $(CFLIB_OBJS) $(PORT_OBJS) >/dev/null 2>$(B)/compile.err; \
	 echo "QED:   $$(ls $(B)/qed/*.o 2>/dev/null | wc -l) of $(words $(QED_SRCS)) compiled"; \
	 echo "cflib: $$(ls $(B)/cflib/*.o 2>/dev/null | wc -l) of $(words $(CFLIB_SRCS)) compiled"; \
	 echo "port:  $$(ls $(B)/port/*.o 2>/dev/null | wc -l) of $(words $(PORT_OBJS)) compiled"; \
	 grep -a 'error:' $(B)/compile.err | sed 's/^/  /' | sort | uniq | head -60

clean:
	rm -rf $(B)

.PHONY: all compile check demo clean
.DEFAULT_GOAL := all
