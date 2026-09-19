/* qed4xe.c -- what QED and cflib call that the kit does not serve,
 * answered for this machine.  Declared in include/qed4xe.h, which every
 * file reaches through include/gem.h.
 *
 * The rule: each answer is TRUE of an Atari XL/XE running gem4xe under
 * SpartaDOS X, one application at a time, with no GDOS and no MiNT -- not
 * the smallest thing that links.  cflib4xe's FINDINGS.md 30 has the four
 * cases where the linkable answer would have sent a program off to read
 * an address that means nothing here, or lay out its dialogs to a font
 * height the machine does not have. */
#include <gem.h>
#include <cflib.h>          /* NK_*, NKF_*; declares gem_to_norm, nkc_init, select_file */
#include <tos.h>            /* cflib/tos.h: CF_GEMPARBLK, the shim's declarations */
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define EINVFN  (-32L)      /* GEMDOS: no such function -- what TOS says to a MiNT call */
#define ENMFIL  (-49L)      /* GEMDOS: no more files */

/* -- gemlib's parameter block, for cflib ---------------------------------- */

/* cflib reads global[0] through _GemParBlk (appinit.c, xgetinfo.c); the
 * kit has the array as `global`. */
CF_GEMPARBLK _GemParBlk = { global };

/* -- MiNT ---------------------------------------------------------------- */

LONG Pdomain(WORD domain)
{
    (void)domain;
    return EINVFN;                  /* TOS has no domains; QED discards this */
}

LONG qed4xe_Psignal(WORD sig, long handler)
{
    (void)sig; (void)handler;
    return EINVFN;                  /* and no signals; QED discards this too */
}

/* -- the AES calls that assume other applications ----------------------- */

WORD appl_find(const char *name)
{
    (void)name;
    return -1;                      /* no application of that name: there is one, and it is us */
}

WORD appl_search(WORD mode, char *name, WORD *type, WORD *ap_id)
{
    (void)mode; (void)name; (void)type; (void)ap_id;
    return 0;                       /* no (more) entries in the process list */
}

/* -- the AES calls cflib reaches that assume a richer AES ---------------- */

WORD objc_sysvar(WORD mode, WORD which, WORD in1, WORD in2,
                 WORD *out1, WORD *out2)
{
    (void)mode; (void)which; (void)in1; (void)in2;
    if (out1) *out1 = 0;            /* no 3D frame inflation on this AES */
    if (out2) *out2 = 0;
    return 0;
}

WORD objc_change_grect(OBJECT *tree, WORD obj, WORD depth, const GRECT *r,
                       WORD newstate, WORD redraw)
{
    return objc_change(tree, obj, depth, r->g_x, r->g_y, r->g_w, r->g_h,
                       newstate, redraw);
}

WORD menu_popup(MENU *m1, WORD x, WORD y, MENU *m2)
{
    (void)m1; (void)x; (void)y; (void)m2;
    return 0;                       /* not served; cf_menu_popup never reaches here */
}

WORD Fselect(WORD timeout, LONG *rfds, LONG *wfds, LONG *xfds)
{
    (void)timeout; (void)wfds; (void)xfds;
    if (rfds) *rfds = 0;            /* nothing is ever ready: no pipes, no second app */
    return 0;
}

/* -- GDOS printing and the WDialog printer dialog ----------------------- */

/* Never reached at run time: vq_vgdos answers -2 and appl_xgetinfo(7)/(9)
 * answer 0, so QED's own tests keep it off these paths.  Each answers as
 * the call answers when the facility is absent. */
PRN_DIALOG *pdlg_create(WORD flags)                     { (void)flags; return 0; }
WORD  pdlg_delete(PRN_DIALOG *pd)                       { (void)pd; return 0; }
WORD  pdlg_open(PRN_DIALOG *pd, PRN_SETTINGS *settings, const char *document,
                WORD flags, WORD x, WORD y)
{
    (void)pd; (void)settings; (void)document; (void)flags; (void)x; (void)y;
    return 0;
}
WORD  pdlg_close(PRN_DIALOG *pd, WORD *x, WORD *y)      { (void)pd; (void)x; (void)y; return 0; }
WORD  pdlg_evnt(PRN_DIALOG *pd, PRN_SETTINGS *settings, EVNT *events, WORD *button)
{
    (void)pd; (void)settings; (void)events;
    if (button) *button = 0;
    return 0;                       /* the dialog is finished */
}
PRN_SETTINGS *pdlg_new_settings(PRN_DIALOG *pd)         { (void)pd; return 0; }
WORD  pdlg_free_settings(PRN_SETTINGS *settings)        { (void)settings; return 0; }
WORD  pdlg_use_settings(PRN_DIALOG *pd, PRN_SETTINGS *settings) { (void)pd; (void)settings; return 0; }
WORD  pdlg_add_sub_dialogs(PRN_DIALOG *pd, PDLG_SUB *sub) { (void)pd; (void)sub; return 0; }
WORD  pdlg_remove_sub_dialogs(PRN_DIALOG *pd)           { (void)pd; return 0; }

WORD v_opnprn(WORD aes_handle, PRN_SETTINGS *settings, WORD *work_out)
{
    (void)aes_handle; (void)settings; (void)work_out;
    return 0;                       /* no printer workstation opened */
}

void vq_devinfo(WORD handle, WORD device, WORD *dev_exists,
                char *file_name, char *device_name)
{
    (void)handle; (void)device;
    if (dev_exists)  *dev_exists = 0;
    if (file_name)   *file_name = '\0';
    if (device_name) *device_name = '\0';
}

WORD vs_document_info(WORD handle, WORD type, const char *s, WORD wchar)
{
    (void)handle; (void)type; (void)s; (void)wchar;
    return 0;
}

WORD vqt_ext_name(WORD handle, WORD index, char *name, WORD *type, WORD *idx)
{
    (void)handle; (void)index; (void)type; (void)idx;
    if (name) *name = '\0';
    return 0;                       /* no GDOS: no loadable font names */
}

/* -- cflib functions whose home files are not built --------------------- */
/* Each is declared in <cflib.h>; the file that defined it is dropped
 * (Makefile CFLIB_DROP names why).  These are the replacements. */

/* do_alert / do_walert -- cflib's alerts.c builds its own alert dialog from
 * an inline-resource tree (cf_alert_box) and runs it.  gem4xe has the AES's
 * own form_alert, which is exactly this and better; QED's two do_alert calls
 * are fatal-error messages, and do_walert is a general alert.  The window
 * title and the undo-button argument have no place in form_alert and are
 * dropped, as the ST's own form_alert has neither. */
WORD do_alert(WORD def, WORD undo, char *str)
{
    (void)undo;
    return form_alert(def, str);
}

WORD do_walert(WORD def, WORD undo, char *str, char *win_title)
{
    (void)undo; (void)win_title;
    return form_alert(def, str);
}

/* ascii_table / set_asciitable_strings -- cflib's insert-a-character-by-code
 * dialog, whose object tree lives in the inline-resource system this port
 * does not bring up.  The editor is whole without it: ascii_table answers
 * -1 (cancelled), so QED inserts nothing and reports the feature declined,
 * and the strings it would label the dialog with are ignored. */
WORD ascii_table(WORD id, WORD pts)
{
    (void)id; (void)pts;
    return -1;
}

void set_asciitable_strings(char *title, char *button)
{
    (void)title; (void)button;
}

/* fix_dial / fix_menu / fix_popup -- cflib's userdef.c converts MagiC-style
 * radio/check/group-box/underline objects and menu separators into
 * G_USERDEF objects drawn by its own callbacks.  gem4xe draws no G_USERDEF,
 * so running these would make those elements invisible; NOT running them
 * leaves each object as its original AES-drawable type (a plain box, string
 * or button, a disabled "----" separator).  So the no-op is the correct
 * rendering here, not a missing feature. */
void fix_dial(OBJECT *tree)               { (void)tree; }
void fix_menu(OBJECT *tree)               { (void)tree; }
void fix_popup(OBJECT *tree, WORD thin)   { (void)tree; (void)thin; }

/* do_fontsel -- cflib's font selector (fontsel.c), reached only from QED's
 * GDOS printing path, which never runs (vq_vgdos answers -2).  One system
 * font here; 0 is "cancelled", the id and size left as they were. */
WORD do_fontsel(WORD flags, char *title, WORD *id, WORD *pts)
{
    (void)flags; (void)title; (void)id; (void)pts;
    return 0;
}

/* grect_to_array -- gemlib's GRECT-to-pxy helper (not in cflib at all;
 * cflib's window.c and colorpop.c call it).  The VDI's array form of a
 * rectangle: x, y, and the inclusive far corner. */
WORD *grect_to_array(const GRECT *g, WORD *pxy)
{
    pxy[0] = g->g_x;
    pxy[1] = g->g_y;
    pxy[2] = (WORD)(g->g_x + g->g_w - 1);
    pxy[3] = (WORD)(g->g_y + g->g_h - 1);
    return pxy;
}

/* -- MiNT and BIOS, answered as plain TOS answers them ------------------- */

LONG Pgetuid(void)                          { return EINVFN; }   /* -32; QED maps to uid 0 */
LONG Pgetgid(void)                          { return EINVFN; }
LONG Fchmod(const char *name, WORD mode)    { (void)name; (void)mode; return EINVFN; }
LONG Fchown(const char *name, WORD uid, WORD gid) { (void)name; (void)uid; (void)gid; return EINVFN; }
LONG Setprt(WORD config)                    { (void)config; return 0; }

/* -- the keyboard ---------------------------------------------------------- */

/* What gem4xe's keyboard driver delivers (gem4xe src/vdi/vdi.c,
 * kb_translate): a GEM key code with a PC scan code in the high byte for
 * the keys GEM switches on, and for every other key the character alone,
 * scan code zero.  The modifier state comes beside it in kstate. */
#define SC_ESC    0x01
#define SC_BS     0x0E
#define SC_TAB    0x0F
#define SC_RET    0x1C
#define SC_UP     0x48
#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_DOWN   0x50
#define SC_DEL    0x53

/* Keytbl -- three tables with exactly the entries above, and zero for the
 * scan codes this keyboard never sends.  cflib's menu code (menuisk.c)
 * reads the caps table by a key's scan code to match a shortcut letter;
 * with gem4xe's letters carrying no scan code it will match none, which
 * is the truth of the matter, and QED's own shortcut table (kurzel.c)
 * works from the normalised code, below, rather than from these. */
static unsigned char kt_unshift[128], kt_shift[128], kt_caps[128];
static KEYTAB kt = { kt_unshift, kt_shift, kt_caps };
static int kt_ready;

KEYTAB *Keytbl(void *unshift, void *shift, void *capslock)
{
    (void)unshift; (void)shift; (void)capslock;    /* (void *)-1 each: only asking */
    if (!kt_ready) {
        static const unsigned char pairs[] = {
            SC_RET, 0x0D, SC_TAB, 0x09, SC_BS, 0x08, SC_ESC, 0x1B, SC_DEL, 0x7F
        };
        WORD i;
        for (i = 0; i < (WORD)sizeof(pairs); i += 2) {
            kt_unshift[pairs[i]] = pairs[i + 1];
            kt_shift[pairs[i]]   = pairs[i + 1];
            kt_caps[pairs[i]]    = pairs[i + 1];
        }
        kt_ready = 1;
    }
    return &kt;
}

/* nkc_init -- cflib's builds its own copy of Keytbl's answer; there is
 * nothing here to build.  QED calls it once and discards the result. */
_WORD nkc_init(void)
{
    return 0;
}

/* gem_to_norm -- a key event as evnt_multi hands it (kstate, kreturn) to
 * a normalised key code, the form QED's editor and shortcut tables are
 * written against (cflib.h: NK_*, NKF_*).
 *
 * This replaces cflib's nkccgton.c, which feeds the TOS-shaped code to
 * NKCC's nkc_tos2n and needs the BIOS key tables to do it.  Traced
 * through that function rather than assumed: a code with scan code zero
 * comes back as the bare character with its CTRL flag dropped, and a
 * RETURN looked up in empty tables comes back as nothing at all.  So the
 * conversion is done here directly against what gem4xe sends, producing
 * what NKCC would have produced from the ST's tables for the same key:
 *
 *   a printable character        the character, SHIFT if the key is held
 *   Ctrl and a letter            NKF_CTRL|NKF_FUNC|NKF_RESVD and the
 *                                CAPITAL letter, which is NKCC's form for
 *                                it (its caps table wins when Control is
 *                                down); gem4xe delivers such a key as the
 *                                control-row character $01..$1A
 *   the keys with a scan code    NKF_FUNC and the NK_ name, modifiers kept */
unsigned short gem_to_norm(_WORD ks, _WORD kr)
{
    unsigned short scan  = (unsigned short)((kr >> 8) & 0xFF);
    unsigned short ascii = (unsigned short)(kr & 0xFF);
    unsigned short flags = 0;

    if (ks & (K_LSHIFT | K_RSHIFT)) flags |= NKF_LSH;    /* the one shift key reads as left */
    if (ks & K_CTRL)                flags |= NKF_CTRL;
    if (ks & K_ALT)                 flags |= NKF_ALT;

    switch (scan) {
    case SC_RET:   return (unsigned short)(NKF_FUNC | flags | NK_RET);
    case SC_TAB:   return (unsigned short)(NKF_FUNC | flags | NK_TAB);
    case SC_BS:    return (unsigned short)(NKF_FUNC | flags | NK_BS);
    case SC_ESC:   return (unsigned short)(NKF_FUNC | flags | NK_ESC);
    case SC_UP:    return (unsigned short)(NKF_FUNC | flags | NK_UP);
    case SC_DOWN:  return (unsigned short)(NKF_FUNC | flags | NK_DOWN);
    case SC_LEFT:  return (unsigned short)(NKF_FUNC | flags | NK_LEFT);
    case SC_RIGHT: return (unsigned short)(NKF_FUNC | flags | NK_RIGHT);
    case SC_DEL:   return (unsigned short)(NKF_FUNC | flags | NK_DEL);
    default:       break;
    }

    if (ascii >= 0x01 && ascii <= 0x1A)             /* Ctrl-A .. Ctrl-Z */
        return (unsigned short)(NKF_CTRL | NKF_FUNC | NKF_RESVD | (ascii + 0x40));
    if (ascii < 0x20)                               /* another control-row code */
        return (unsigned short)(NKF_FUNC | flags | ascii);
    return (unsigned short)(ascii | flags);
}

/* norm_to_gem -- the inverse of gem_to_norm: a normalised key code back to
 * the kstate/kreturn pair gem4xe's evnt_multi would have delivered.  QED's
 * makro.c calls it to replay a recorded macro key.  Replaces cflib's
 * nkccntog.c (norm_to_gem -> nkc_n2tos through the BIOS tables), which is
 * dropped with the rest of NKCC's TOS-table path; kept the inverse of what
 * gem_to_norm produces, so a recorded key round-trips exactly. */
void norm_to_gem(unsigned long norm, _WORD *ks, _WORD *kr)
{
    unsigned short n = (unsigned short)norm;
    unsigned short low = (unsigned short)(n & 0xFF);
    _WORD state = 0;
    _WORD ret;

    if (n & NKF_LSH)  state |= K_LSHIFT;
    if (n & NKF_RSH)  state |= K_RSHIFT;
    if ((n & NKF_SHIFT) == NKF_SHIFT) state |= K_LSHIFT;   /* "any shift" */
    if (n & NKF_CTRL) state |= K_CTRL;
    if (n & NKF_ALT)  state |= K_ALT;

    if (n & NKF_FUNC) {
        switch (low) {                              /* gem4xe's scan<<8 | ascii */
        case NK_RET:   ret = 0x1C0D; break;
        case NK_TAB:   ret = 0x0F09; break;
        case NK_BS:    ret = 0x0E08; break;
        case NK_ESC:   ret = 0x011B; break;
        case NK_UP:    ret = 0x4800; break;
        case NK_DOWN:  ret = 0x5000; break;
        case NK_LEFT:  ret = 0x4B00; break;
        case NK_RIGHT: ret = 0x4D00; break;
        case NK_DEL:   ret = 0x537F; break;
        default:       ret = (_WORD)low; break;     /* another control code */
        }
    } else if ((n & NKF_CTRL) && low >= 'A' && low <= 'Z') {
        ret = (_WORD)(low - 0x40);                  /* Ctrl-letter -> $01..$1A */
    } else {
        ret = (_WORD)low;                           /* a plain character */
    }

    if (ks) *ks = state;
    if (kr) *kr = ret;
}

/* -- itoa, which mintlib has and ISO C does not.  The kit carries
 * stricmp/strnicmp/strlwr and opendir/readdir/closedir now
 * (gem4xe include/support.h, include/dirent.h), so only this is
 * left here. */
char *itoa(int value, char *buf, int radix)
{
    return ltoa((long)value, buf, radix);
}

/* -- the file selector ----------------------------------------------------- */

/* cflib's select_file (filesel.c) chooses between Selectric, MagiC's
 * fslx_* and the AES's own selector; filesel.c is not built here, because
 * the two extensions do not exist and the AES's path is the whole of what
 * is left.  This is that path, with the same handling of the arguments:
 * path arrives as a directory (or empty, for the current one), the mask
 * is appended for the selector and cut off again after, and the callback
 * -- QED's file.c uses it -- is given the split result and the buffers
 * are emptied for it, as cflib does. */
int select_file(char *path, char *name, char *mask, char *title, FSEL_CB open_cb)
{
    _WORD but = 0;
    char *p;
    int ok;

    if (path[0] == '\0')
        get_path(path, 0);
    else
        make_normalpath(path);

    strcat(path, mask[0] ? mask : "*.*");

    wind_update(BEG_UPDATE);
    fsel_exinput(path, name, &but, title);
    wind_update(END_UPDATE);

    ok = (but == 1);
    if (ok) {
        p = strrchr(path, '\\');
        if (p)
            p[1] = '\0';
        make_normalpath(path);
        if (open_cb) {
            open_cb(path, name);
            path[0] = '\0';
            name[0] = '\0';
        }
    }
    return ok;
}
