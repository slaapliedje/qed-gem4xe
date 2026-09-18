/* macros.h -- mintlib's, as far as QED reaches into it: min and max
 * (global.h includes it for those).  PATH_MAX is a limits.h name mintlib
 * has and Calypsi's does not; dd.c reaches it.  QED's own PATH type is
 * 128 characters (qed.h), which is what a GEMDOS path can be here. */
#ifndef QED4XE_MACROS_H
#define QED4XE_MACROS_H

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef PATH_MAX
#define PATH_MAX 128
#endif

#endif /* QED4XE_MACROS_H */
