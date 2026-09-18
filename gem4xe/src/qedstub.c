/* qedstub.c -- the Calypsi C-library stub the kit does not carry.
 *
 * The kit's lib/gemstub.c provides the nine file stubs (_Stub_read and
 * its companions) over GEMDOS, so printf links.  It does not provide
 * _Stub_assert, which Calypsi's libc calls when an assert() fails; QED's
 * hl.c asserts on its highlight pointers.  Without it the link fails with
 *
 *     missing stub routine '_Stub_assert' needs to be provided for your
 *     hardware/board-support
 *
 * which names the board, not the program.  A failed assertion is a bug,
 * so this says where -- on GEM's VT-52 console, through the kit's Cconws
 * -- and ends the program with Pterm, which is what abort means here:
 * gem4xe's shell takes the code and puts the desktop back.  It must not
 * return (Calypsi marks it __noreturn_function), and Pterm does not. */
#include <gem.h>

static void put_num(WORD n)
{
    char buf[8];
    WORD i = 0, j;
    if (n < 0) { Cconout('-'); n = (WORD)-n; }
    do { buf[i++] = (char)('0' + n % 10); n = (WORD)(n / 10); } while (n && i < 8);
    for (j = i; j > 0; j--)
        Cconout(buf[j - 1]);
}

void _Stub_assert(const char *filename, int linenum)
{
    Cconws("\r\nqed: assertion failed at ");
    if (filename)
        Cconws(filename);
    Cconout(':');
    put_num((WORD)linenum);
    Cconws("\r\n");
    Pterm(1);
    for (;;)                        /* Pterm does not return; satisfy noreturn */
        ;
}

/* _Stub_environ -- Calypsi's getenv() asks the board for the environment
 * array.  QED reaches getenv (clipbrd.c, options.c, av.c, olga.c, global.c)
 * for things like a HOME or a TEMP directory; there is no environment on
 * gem4xe, so the array is empty and getenv answers 0 for everything.  A
 * single NULL-terminated, empty vector is the honest "no variables set". */
static char *qed_environ[1] = { 0 };

char **_Stub_environ(void)
{
    return qed_environ;
}
