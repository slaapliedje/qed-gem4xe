/* qed4xe.h -- what QED and cflib call that the kit does not serve, declared
 * here and answered in src/qed4xe.c.  Every translation unit reaches this
 * through include/gem.h, after the kit's gem.h.
 *
 * The rule for everything in here, learned by cflib4xe and written into
 * the kit's README: a stub answers TRUTHFULLY for this machine, not merely
 * in a way that links.  "No" is a fine answer; a plausible wrong one is
 * what turns a missing feature into a bug that looks like something else. */
#ifndef QED4XE_H
#define QED4XE_H

#include <stddef.h>
/* stricmp, strnicmp, strlwr, strupr: the kit HAS them (gemcompat.c), but
 * an ST source gets them out of <string.h> and so never asks for a header
 * of its own.  av.c, dd.c and text.c were calling them through an
 * IMPLICIT declaration -- which linked here only because Calypsi's int
 * is sixteen bits and happens to be what they return.  One include, at
 * the one door every translation unit already passes. */
#include <support.h>

/* -- names the kit spells differently ---------------------------------- */

/* The kit has the Compendium's abbreviations; the ST's headers, and QED's
 * window.c, spell the two slider-size fields out. */
#ifndef WF_HSLSIZE
#define WF_HSLSIZE  WF_HSLSIZ           /* 15 */
#define WF_VSLSIZE  WF_VSLSIZ           /* 16 */
#endif

/* SH_WDRAW (72) -- shel_write's "redraw the desktop window" opcode, which
 * QED's event.c passes on.  There is no second application to ask; the
 * name is what event.c needs to compile. */
#ifndef SH_WDRAW
#define SH_WDRAW  72
#endif

/* AES 4's drag-and-drop message.  cflib's ddcreate.c sends it to the
 * application it found with appl_find -- which answers a real pid now,
 * but only ever this program's or an accessory's, and neither is a
 * drag-drop server; the name is needed for the file to compile. */
#ifndef AP_DRAGDROP
#define AP_DRAGDROP 63
#endif

/* __GNUC_PREREQ -- QED's kurzel.c guards a GCC #pragma with
 * `#if __GNUC_PREREQ(7, 0)`.  This is not GCC; the function-like macro
 * must exist for the #if to evaluate, and 0 takes the non-GCC arm. */
#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(a, b) 0
#endif

/* -- evnt_multi, gemlib's shape ---------------------------------------- */

/* The kit declares the ST's twenty-three arguments, the timer as two words.
 * QED and cflib are written against gemlib, which passes it as ONE long:
 * twenty-two.  The split is the kit's own (evnt_multi_moblk writes int_in[14]
 * = low, [15] = high), and this is cflib4xe's adapter, moved to the door
 * every file passes through.  Object-like, so the argument list is left
 * alone; it must follow the kit's declaration. */
static WORD qed4xe_evnt_multi(WORD fl, WORD bc, WORD bm, WORD bs,
                              WORD m1f, WORD m1x, WORD m1y, WORD m1w, WORD m1h,
                              WORD m2f, WORD m2x, WORD m2y, WORD m2w, WORD m2h,
                              WORD *msg, LONG ti,
                              WORD *mx, WORD *my, WORD *mb, WORD *ks,
                              WORD *kr, WORD *br)
{
    return evnt_multi(fl, bc, bm, bs, m1f, m1x, m1y, m1w, m1h,
                      m2f, m2x, m2y, m2w, m2h, msg,
                      (WORD)(ti & 0xFFFFL), (WORD)((ti >> 16) & 0xFFFFL),
                      mx, my, mb, ks, kr, br);
}
#define evnt_multi qed4xe_evnt_multi

/* -- MiNT ---------------------------------------------------------------- */

/* Signal numbers as MiNT numbers them; QED's main.c passes them to Psignal
 * and nowhere else.  Calypsi's <signal.h> numbers its own six differently
 * and QED never calls signal(), so nothing meets both. */
#ifndef SIGHUP
#define SIGHUP   1
#endif
#ifndef SIGQUIT
#define SIGQUIT  3
#endif
#ifndef SIGSYS
#define SIGSYS   12
#endif
#ifndef SIGPIPE
#define SIGPIPE  13
#endif

/* Pdomain(1) asks MiNT for its domain; TOS answers EINVFN and QED ignores
 * the answer.  Psignal installs a handler; TOS has none to install into.
 * The handler is taken as a long so that SIG_IGN, a cast integer, and
 * handle_term, a function, both pass without a diagnostic. */
LONG Pdomain(WORD domain);
LONG qed4xe_Psignal(WORD sig, long handler);
#define Psignal(sig, handler) qed4xe_Psignal((sig), (long)(handler))

/* The ST's Frename carries a reserved first word; the kit's binding does
 * not.  QED's file.c calls it the ST's way. */
#define Frename(zero, oldname, newname) (Frename)((oldname), (newname))

/* -- the AES calls that assume other applications ----------------------- */

/* appl_find is the KIT's since gem4xe 751d956: the AES names every
 * process after the file the shell loaded it from and searches them, so
 * a name this program asks for is found if an accessory carries it.
 * appl_search is AES 4.0's, gated there on appl_getinfo, and gem4xe
 * reports 1.40 -- so it stays here, answering what a one-entry list
 * answers. */
WORD appl_search(WORD mode, char *name, WORD *type, WORD *ap_id);

/* -- the AES calls cflib reaches that assume a richer AES ---------------- */

/* objc_change_grect -- gemlib's objc_change taking a GRECT.  cflib's
 * ppmenu.c highlights a popup item through it.  The kit has objc_change,
 * so this is a real wrapper, not a stub: if the popup path ever runs it
 * draws correctly. */
WORD objc_change_grect(OBJECT *tree, WORD obj, WORD depth, const GRECT *r,
                       WORD newstate, WORD redraw);

/* menu_popup (AES 36) -- not served here.  cflib's cf_menu_popup calls it
 * only after appl_xgetinfo(9) reports the AES has it, which answers 0, so
 * it is never reached; the MENU type is the kit's. */
WORD menu_popup(MENU *m1, WORD x, WORD y, MENU *m2);

/* Fselect -- MiNT's select().  cflib's ddcreate.c waits on a drag-drop
 * pipe with it.  There are no pipes and no second application, so nothing
 * is ever ready: 0. */
WORD Fselect(WORD timeout, LONG *rfds, LONG *wfds, LONG *xfds);

/* -- GDOS printing and the WDialog printer dialog ----------------------- */

/* QED's printing goes through GDOS (v_opnprn, vs_document_info) and, when
 * the AES has WDialog's printer dialog, through pdlg_*.  Neither exists
 * here: vq_vgdos answers -2 and appl_xgetinfo(7)/(9) answer 0, so QED
 * never takes these paths at run time -- they must only compile and link.
 *
 * The WDialog types are gemlib's, mirrored in cflib's wdlgpdlg.h /
 * wdlgevnt.h, which are not built here.  QED's prn_cfg.c uses EVNT and a
 * few PDLG_SUB fields, so the shapes are given -- faithful to the ST's
 * offsets, since a memset(sizeof) walks them -- with the driver-internal
 * fields as opaque padding.  Nothing here runs; it links. */

#ifndef __EVNT
#define __EVNT
typedef struct {
    WORD mwhich, mx, my, mbutton, kstate, key, mclicks;
    WORD reserved[9];
    WORD msg[16];
} EVNT;
#endif

/* PRN_SETTINGS: QED holds only a pointer to it, but malloc(sizeof) and
 * memcpy(sizeof) on the dead pdlg path need a complete type.  The ST's
 * _prn_settings is a few hundred bytes of printer state; nothing here
 * reads a field, so it is a box larger than the real struct -- so that a
 * run that cannot happen (pdlg_new_settings answers 0, so QED never calls
 * pdlg_dial) would still not under-allocate. */
typedef struct qed4xe_prn_settings { char opaque[1024]; } PRN_SETTINGS;
typedef struct qed4xe_pdlg_sub PDLG_SUB;

/* The dialog's OK button code, as pdlg_evnt reports it in *button.  Dead
 * here; the value is gemlib's PDLG_PB_OK. */
#ifndef PDLG_OK
#define PDLG_OK 1
#endif

/* The callback triple, plain function pointers (Calypsi has one calling
 * convention, so QED's CDECL-qualified callbacks assign to these). */
typedef long (*PDLG_INIT)(PRN_SETTINGS *settings, PDLG_SUB *sub);
struct PDLG_HNDL_args { PRN_SETTINGS *settings; PDLG_SUB *sub; WORD exit_obj; };
typedef long (*PDLG_HNDL)(struct PDLG_HNDL_args args);
typedef long (*PDLG_RESET)(PRN_SETTINGS *settings, PDLG_SUB *sub);

/* PDLG_SUB, the ST's 96-byte layout (cflib wdlgpdlg.h): the fields QED
 * touches by name, the rest as reserved words at their real offsets. */
struct qed4xe_pdlg_sub {
    /*  0 */ PDLG_SUB   *next;
    /*  4 */ LONG        length, format, reserved0;
    /* 16 */ void       *drivers;
    /* 20 */ WORD        option_flags, sub_id;
    /* 24 */ void       *dialog;
    /* 28 */ OBJECT     *tree;
    /* 32 */ WORD        index_offset, reserved1;
    /* 36 */ LONG        reserved2, reserved3, reserved4;
    /* 48 */ PDLG_INIT   init_dlg;
    /* 52 */ PDLG_HNDL   do_dlg;
    /* 56 */ PDLG_RESET  reset_dlg;
    /* 60 */ LONG        reserved5;
    /* 64 */ OBJECT     *sub_icon, *sub_tree;
    /* 72 */ LONG        reserved6, reserved7;
    /* 80 */ LONG        private1, private2, private3, private4;
};

typedef struct qed4xe_prn_dialog PRN_DIALOG;
#ifndef PDLG_3D
#define PDLG_3D 0x0001
#endif
PRN_DIALOG   *pdlg_create(WORD flags);
WORD          pdlg_delete(PRN_DIALOG *pd);
WORD          pdlg_open(PRN_DIALOG *pd, PRN_SETTINGS *settings,
                        const char *document, WORD flags, WORD x, WORD y);
WORD          pdlg_close(PRN_DIALOG *pd, WORD *x, WORD *y);
WORD          pdlg_evnt(PRN_DIALOG *pd, PRN_SETTINGS *settings, EVNT *events,
                        WORD *button);
PRN_SETTINGS *pdlg_new_settings(PRN_DIALOG *pd);
WORD          pdlg_free_settings(PRN_SETTINGS *settings);
WORD          pdlg_use_settings(PRN_DIALOG *pd, PRN_SETTINGS *settings);
WORD          pdlg_add_sub_dialogs(PRN_DIALOG *pd, PDLG_SUB *sub);
WORD          pdlg_remove_sub_dialogs(PRN_DIALOG *pd);
WORD  v_opnprn(WORD aes_handle, PRN_SETTINGS *settings, WORD *work_out);
void  vq_devinfo(WORD handle, WORD device, WORD *dev_exists,
                 char *file_name, char *device_name);
WORD  vs_document_info(WORD handle, WORD type, const char *s, WORD wchar);
WORD  vqt_ext_name(WORD handle, WORD index, char *name, WORD *type, WORD *idx);

/* -- MiNT and BIOS calls with a plain-TOS answer ------------------------- */

/* Pgetuid/Pgetgid: TOS has no user ids and answers EINVFN (-32); QED's
 * global.c tests for exactly -32 and treats it as uid 0. */
LONG Pgetuid(void);
LONG Pgetgid(void);
/* Fchmod/Fchown: no permissions or ownership on GEMDOS; EINVFN.  QED's
 * file.c reaches Fchown only when Pgetuid()==0, which -32 never is. */
LONG Fchmod(const char *name, WORD mode);
LONG Fchown(const char *name, WORD uid, WORD gid);
/* Setprt(-1) reads the printer-config word; QED's prn_out.c tests bit 4.
 * There is no such word here: 0. */
LONG Setprt(WORD config);

/* -- BIOS and XBIOS -------------------------------------------------------- */

/* Bconout(2, 7) is the bell, twenty-eight times in QED; there is no bell.
 * Implemented in cflib/cflibc.c (cflib4xe's). */
WORD Bconout(WORD dev, WORD c);

/* Keytbl -- XBIOS 16, the keyboard's scancode-to-ASCII tables.  cflib's
 * menu code (menucre.c) and its form_do (fdfindsc.c) look shortcut
 * letters up in them.  gem4xe's keyboard driver delivers a scan code only
 * for the few keys GEM switches on (RETURN, TAB, BACKSPACE, ESC, the
 * arrows, DELETE) and plain ASCII with none for the rest, so the tables
 * answered are exactly that: those entries, and zero elsewhere.  A copy
 * of the ST's tables would claim scan codes this keyboard never sends. */
typedef struct {
    unsigned char *unshift;
    unsigned char *shift;
    unsigned char *capslock;
} KEYTAB;
KEYTAB *Keytbl(void *unshift, void *shift, void *capslock);

/* -- non-ISO string functions QED reaches -------------------------------- */

char *itoa(int value, char *buf, int radix);
char *ltoa(long value, char *buf, int radix);           /* cflib/cflibc.c */

#endif /* QED4XE_H */
