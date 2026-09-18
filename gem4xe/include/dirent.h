/* dirent.h -- opendir/readdir/closedir on gem4xe, over one Fsfirst and a
 * run of Fsnext.
 *
 * QED reads directories in two places: hl.c lists the syntax files and
 * projekt.c walks a project's directory.  cflib has a dirent.c of its own,
 * over MiNT's Dopendir with an Fsfirst fallback; the kit has no Dopendir,
 * so rather than stub three MiNT calls to fail on the way to the fallback,
 * the fallback is written here as the only path (src/qed4xe.c).
 *
 * A DIR carries its own DTA: the kit's Fsfirst writes the DTA the caller
 * sets, and QED may hold two directories open at once, so each keeps its
 * own and sets it before every Fsnext. */
#ifndef QED4XE_DIRENT_H
#define QED4XE_DIRENT_H

#include <gem.h>

struct dirent {
    char d_name[14];            /* 8.3 and a NUL, as GEMDOS gives it */
};

typedef struct {
    DTA           dta;          /* this directory's search state */
    struct dirent ent;          /* what readdir last answered */
    WORD          first;        /* 1 until the Fsfirst result has been handed out */
    WORD          done;         /* 1 once Fsnext has said there is no more */
} DIR;

DIR           *opendir(const char *path);
struct dirent *readdir(DIR *d);
int            closedir(DIR *d);

#endif /* QED4XE_DIRENT_H */
