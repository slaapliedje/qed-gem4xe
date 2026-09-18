/* tos.h -- the compatibility shim cflib's intern.h reaches under Calypsi.
 *
 * This is cflib4xe's src/compat/tos.h (~/dev/cflib4xe), with three changes
 * for this build: the kit is reached through <gem.h> -- which is
 * include/gem.h, the kit's plus qed4xe.h -- rather than by a path into
 * cflib4xe's own kit snapshot; the evnt_multi adapter and KEYTAB have
 * moved to qed4xe.h, where QED's files see them too; and Keytbl is
 * declared, which cflib4xe left implicit -- and an implicit declaration
 * returns int, sixteen bits, where a pointer here is thirty-two.
 *
 * What it is for: cflib was written against FreeMiNT's gemlib.  Its
 * intern.h branches: under __GNUC__ it takes <mint/osbind.h>,
 * <mint/mintbind.h> and <gemx.h>; otherwise it takes <tos.h> and nothing
 * else.  Calypsi defines __CALYPSI__ and NOT __GNUC__, so this file is the
 * whole of cflib's system-header surface -- measured by cflib4xe, not
 * assumed (its FINDINGS.md 6).
 *
 * Port code, not cflib: cflib's sources compile from the freemint/cflib
 * checkout unmodified. */
#ifndef QED4XE_CFLIB_TOS_H
#define QED4XE_CFLIB_TOS_H

#include <gem.h>

/* _WORD/_UWORD are defined in cflib.h only under __GNUC__ or __PUREC__, so
 * under Calypsi they arrive undefined.  The build passes them as -D rather
 * than defining them here, because cflib.h is reached before this file is.
 * _LONG needs nothing: cflib.h falls through to `long`, the kit's LONG. */

/* gemlib exposes the AES parameter block as _GemParBlk.  cflib reads
 * exactly one thing from it -- global[0], the AES version -- at appinit.c:46
 * and xgetinfo.c:39.  The kit publishes the same array as a plain `global`. */
typedef struct {
    WORD *global;
} CF_GEMPARBLK;

extern CF_GEMPARBLK _GemParBlk;

/* _AESversion -- gemlib's name for the AES version.  The AES fills
 * global[0] with it (0x0140 here), and the kit declares global[]. */
#define _AESversion  global[0]

/* appl_xgetinfo -- gemlib's, which cflib expects to exist: its own
 * xgetinfo.c is entirely inside #if __PUREC__ && !_GEMLIB_COMPATIBLE and so
 * compiles to nothing here.  cflib.h:377 guards the declaration with
 * #ifndef appl_xgetinfo, so ours may own the name.  It answers the font
 * queries from graf_handle -- see cflibc.c for why a plain `return 0`
 * would lay every dialog out to a font height this machine does not have. */
WORD appl_xgetinfo(WORD type, WORD *out1, WORD *out2, WORD *out3, WORD *out4);

/* DTA member spellings.  The kit's DTA is the ST's -- d_attrib, d_time,
 * d_date, d_length, d_fname -- and cflib reaches for mintlib's.  Same 44
 * bytes in the same order.  cflib4xe checked that these five names occur
 * in cflib only as DTA member accesses, never as identifiers of their own. */
#define dta_attribute  d_attrib
#define dta_time       d_time
#define dta_date       d_date
#define dta_size       d_length
#define dta_name       d_fname

/* Non-ISO libc and BIOS entries cflib reaches; implemented in cflibc.c. */
char *ltoa(long value, char *buf, int radix);
char *ultoa(unsigned long value, char *buf, int radix);
void  unx2dos(const char *unx, char *dos);
WORD  Bconout(WORD dev, WORD c);
WORD  Getrez(void);
extern WORD gl_apid;
WORD  vq_vgdos(void);
LONG  Dpathconf(const char *path, WORD mode);
void  init_userdef(void);
void  exit_userdef(void);

#endif /* QED4XE_CFLIB_TOS_H */
