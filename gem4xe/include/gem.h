/* gem.h -- what <gem.h> resolves to in this build: the kit's own, and then
 * the port's additions.
 *
 * Every translation unit reaches <gem.h>: QED's through global.h ->
 * <cflib.h> -> <gem.h>, or through <gemx.h>; cflib's through cflib.h and
 * intern.h -> <tos.h>.  Calypsi has no -include, so this is the one door
 * through which the port can put a declaration in front of all of them.
 * The kit is snapshotted to build/kit by the Makefile; the path is relative
 * to this file so that no -I ordering can make the two gem.h shadow each
 * other (the shape cflib4xe's src/compat/gem.h uses, for the same reason). */
#ifndef QED4XE_GEM_H
#define QED4XE_GEM_H

#include "../build/kit/include/gem.h"
#include "qed4xe.h"

#endif /* QED4XE_GEM_H */
