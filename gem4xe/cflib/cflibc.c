/* cflibc.c -- the handful of non-ISO libc and BIOS entries cflib reaches
 * that are the port's business rather than the kit's.
 *
 * Part of cflib4xe. Port code, not cflib. See ../../FINDINGS.md 13.
 */
#include "tos.h"
#include <string.h>

/* ltoa/ultoa are Pure C and mintlib extensions, not ISO. cflib's intern.h
 * maps them onto _ltoa/_ultoa, but only under __GNUC__ && __MINT__, so
 * under Calypsi the bare names are what four call sites reach:
 * obslong.c:34, obsshort.c:34 (ltoa) and obsulong.c:34 (ultoa), every one
 * of them with radix 10. */
static char *cf_utoa(unsigned long v, char *buf, int radix, int neg)
{
    char tmp[34];
    int i = 0;
    char *p = buf;

    if (radix < 2 || radix > 36) { *buf = '\0'; return buf; }
    do {
        unsigned long d = v % (unsigned long)radix;
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + (d - 10));
        v /= (unsigned long)radix;
    } while (v != 0);

    if (neg) *p++ = '-';
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';
    return buf;
}

char *ltoa(long value, char *buf, int radix)
{
    if (value < 0 && radix == 10)
        return cf_utoa((unsigned long)(-value), buf, radix, 1);
    return cf_utoa((unsigned long)value, buf, radix, 0);
}

char *ultoa(unsigned long value, char *buf, int radix)
{
    return cf_utoa(value, buf, radix, 0);
}

/* unx2dos -- mintlib's. cflib reaches it once, fsnorm.c:44, and only when
 * a path begins with '/'. GEMDOS paths are DOS-shaped, so this converts
 * separators and nothing more.
 *
 * NOT done: mintlib also maps a leading /dev/<letter>/ onto a drive. There
 * is no /dev here, so a path in that form would come out wrong rather than
 * be rejected. Left as-is deliberately: inventing a drive mapping gem4xe
 * does not have would be worse than not handling a path it cannot receive. */
void unx2dos(const char *unx, char *dos)
{
    const char *s = unx;
    char *d = dos;

    while (*s) {
        *d++ = (*s == '/') ? '\\' : *s;
        s++;
    }
    *d = '\0';
}

/* Bconout -- BIOS 3. gem4xe has no BIOS call gate, and does not need one
 * for this: ALL SIX of cflib's uses are Bconout(2, 7), device 2 being the
 * console and 7 being BEL. It is a beep, and cfformdo.c:84 says so in a
 * comment. asciitab.c:188, cfformdo.c:84, mddo.c:66/77/116, wdclick.c:65.
 *
 * gem4xe has no console bell, so this is a no-op. That is a real, if small,
 * behavioural difference: cflib beeps at a rejected key in a dialog, and
 * here it will not. Recorded rather than hidden. */
WORD Bconout(WORD dev, WORD c)
{
    (void)dev; (void)c;
    return 0;
}

/* Getrez -- XBIOS 4. cflib reaches it once, appinit.c:72, and NOT as a
 * cosmetic test: it is the fallback that picks the system font height when
 * appl_xgetinfo() fails --
 *
 *     if ((Getrez() == 0) || (Getrez() == 1))   ST-Low/Mid
 *         sys_big_height = 6;
 *     else
 *         sys_big_height = 13;
 *
 * and on gem4xe appl_xgetinfo IS a plain `return 0`, so this path always
 * runs. Whatever this answers sets the font metric cflib lays every dialog
 * out with.
 *
 * NOW UNREACHABLE on the appinit path, and deliberately so: our
 * appl_xgetinfo answers the font query from graf_handle, so cflib never
 * takes this fallback. gem4xe's cell is 8x8 -- neither 6 nor 13 -- so
 * there was no right constant to choose here, which is why the fix went
 * into appl_xgetinfo instead. This remains only so appinit.c links.
 *
 * (Was PROVISIONAL at 2, meaning ST-High, giving 13.) There is no third branch to
 * take, so some value must be chosen, and 13 is the safer of the two for a
 * machine that is not a 16-colour ST mode. But the RIGHT answer is gem4xe's
 * actual system font height, which the kit does not state statically -- it
 * is a runtime vqt_fontinfo/vst_height query. If gem4xe's font is not 13
 * tall, every cflib dialog will be laid out to the wrong metric, and it
 * will look like a dialog bug rather than a constant.
 *
 * Asked of gem4xe; see FINDINGS.md. Do not treat this as settled. */
WORD Getrez(void)
{
    return 2;
}

/* appl_xgetinfo -- see tos.h for why this exists and why it must not be a
 * plain `return 0`. Types 0 and 1 are gemlib's AES_LARGEFONT and
 * AES_SMALLFONT: out1 the cell height, out2 the font id. gem4xe has one
 * face, so both answer the same cell, taken live from graf_handle rather
 * than from a constant. */
WORD appl_xgetinfo(WORD type, WORD *out1, WORD *out2, WORD *out3, WORD *out4)
{
    WORD wchar, hchar, wbox, hbox;

    switch (type) {
    case 0:     /* AES_LARGEFONT */
    case 1:     /* AES_SMALLFONT */
        if (graf_handle(&wchar, &hchar, &wbox, &hbox) == 0)
            return 0;                   /* no answer; let cflib fall back */
        if (out1) *out1 = hchar;        /* the device's real cell height */
        if (out2) *out2 = 1;            /* the system font */
        if (out3) *out3 = 0;
        if (out4) *out4 = 0;
        return 1;
    default:
        return 0;                       /* no appl_getinfo on this AES */
    }
}

/* The Calypsi platform stubs that used to live here -- _Stub_write and
 * its six companions over GEMDOS -- are GONE. The kit ships lib/gemstub.c
 * with all nine, so a program that prints links without a shim. That gap
 * (finding 26) is closed; this comment is all that is left of it.
 *
 * Worth knowing when reading debug.c's output on the machine: stdout is
 * unbuffered, so a ten-byte line is ten Fwrites, each a trip through the
 * COP gate. setvbuf with an array of your own fixes it -- there is no
 * heap, so do not let stdio allocate the buffer. */

/* gl_apid -- cflib declares it extern at cflib.h:369 and defines it
 * nowhere; gemlib carries it, as it carries _GemParBlk and _AESversion.
 * appinit.c assigns it from appl_init(). Same class of gap as those two. */
WORD gl_apid;

/* vq_vgdos -- appinit.c:92 does `gl_gdos = vq_vgdos() != -2;`, so -2 is
 * precisely "no GDOS", and gem4xe has none. Answering -2 makes gl_gdos
 * FALSE and skips vst_load_fonts, which is the correct path here rather
 * than a convenient one. */
WORD vq_vgdos(void) { return -2; }

/* Dpathconf -- fscase.c asks mode 6, the case-sensitivity of the
 * filesystem holding a path, and maps the answer: 0 -> FULL_CASE (a real
 * distinction, MinixFS), 2 -> HALF_CASE (preserved but not distinguished,
 * VFAT), anything else -> NO_CASE.
 *
 * gem4xe's filesystem is 8.3 and neither distinguishes nor preserves
 * case, so NO_CASE is the true answer, and -1 ("not available") is the
 * documented way to say so. Deliberately not 0 or 2: either would claim a
 * distinction this machine does not make, and cflib would then compare
 * filenames in a way the filesystem does not. */
LONG Dpathconf(const char *path, WORD mode)
{
    (void)path; (void)mode;
    return -1L;
}

/* init_userdef / exit_userdef -- userdef.c implements user-drawn objects
 * and is not ported: gem4xe draws no G_USERDEF yet, and MControl's
 * resource contains none (86 objects, zero of type 24). appinit.c and
 * appexitg.c call these unconditionally, so they must exist to link. */
void init_userdef(void) { }
void exit_userdef(void) { }
