# QED for gem4xe

[QED](https://github.com/freemint/qed), the GEM text editor from the Atari
ST, built for an **Atari XL/XE running [gem4xe](https://github.com/slaapliedje/gem4xe)**
(a 65C816 accelerator such as a Rapidus, and a VBXE or the stock ANTIC
screen).  QED's own 37 source files are compiled unmodified; everything the
8-bit machine needs is in this directory.

It opens and edits text files, and saves them through the file selector.
It runs as an ordinary program from gem4xe's desktop, which stays in
place underneath it.

## Building

You need:

- **Calypsi 5.18.2 or later** for the 65816 (`CALYPSI`, default
  `~/dev/toolchains/calypsi-65816`).
- **gem4xe's application kit** (`GEM4XE_SDK`): `make sdk` in a gem4xe
  tree, or the `gem4xe-sdk.tar.gz` of a release.
- **cflib** (`CFLIB_SRC`): a checkout of
  [freemint/cflib](https://github.com/freemint/cflib), unmodified.

Then:

    make GEM4XE_SDK=... CFLIB_SRC=...     # build/QED.G4A and build/QED.RSC
    make check                            # boots it under AltirraSDL

Copy `build/QED.G4A` to the Atari as `QED.PRG`, beside `QED.RSC`.
`make check` also needs a gem4xe tree (`GEM4XE`) and the SpartaDOS X
cartridge and floppy named in its `fixtures.toml`.  It builds a disk,
launches QED from the desktop, types into a new document and saves it.

## What is here

| | |
|---|---|
| `Makefile` | the build: QED, cflib and the port, linked against the kit |
| `src/` | the port's own code: what QED and cflib call that the kit does not serve (`qed4xe.c`), and QED's heap (`qedmem.c`) |
| `include/` | what QED's MiNT includes resolve to here |
| `cflib/` | a three-file compatibility shim for cflib |
| `tools/ci_gem4xe.py` | the gate `make check` runs |
| `NOTICE` | the terms of every part of the built program; ship it with the binary |

## Licence

QED is public domain by its author's own words, with one condition: the
program and its sources may not be distributed for a fee of any kind
(Tom Quellenberg, 1994: `../dist/liesmich.txt`, and the English
hypertext `../doc/qed-en.stg`).  The port's own files here are under the
same terms.

The built program also contains gem4xe's application kit and cflib, both
under the LGPL 2.1 or later, and parts of the Calypsi C library, which
come from Apache NuttX under the Apache License 2.0.  `NOTICE` has the
details and must travel with the binary.
