/* qedmem.c -- malloc, free, realloc and calloc for QED on gem4xe.
 *
 * The kit has no C heap and says so at link time: its lib/clib.c defines
 * malloc and free as calls to an undefined symbol named
 * gem4xe_has_no_heap__use_GEMDOS_Malloc, because gem4xe's Malloc is a bump
 * allocator wound back when the program exits, and Mfree is a no-op.  That
 * is the right contract for an accessory and the wrong one for an editor:
 * QED calls malloc in eleven files and free as often, and its own text
 * store (memory.c) takes 52 KB blocks straight from Malloc and manages
 * them itself.  Against a no-op free the far heap would be a long fuse.
 *
 * So, as RetroWP does (retroplat's mem_gem4xe.c): Malloc supplies ARENAS,
 * and a first-fit allocator with coalescing runs inside them.  Malloc is
 * called once per 32 KB and Mfree never, which is what the bump allocator
 * is for.
 *
 * Two things the machine imposes, both load-bearing:
 *
 *   - A far pointer's arithmetic is sixteen bits: p + n never carries
 *     into the bank byte.  gem4xe's Malloc never hands out a block that
 *     crosses a bank, so an arena is asked for at a size that fits one,
 *     and every pointer this file computes stays inside its arena.
 *   - All arenas hang on ONE address-ordered chain, and free coalesces a
 *     block with the free block after it.  Two arenas are not adjacent,
 *     so each ends in a zero-length block marked USED, which no merge can
 *     cross.  The invariant is kept by construction.
 *
 * This is the kit's lib/clib.c left out of the link (Makefile, KIT_OBJS),
 * which QED may do because it is public domain and so may also link
 * Calypsi's own str* functions, the other thing clib.c is for. */
#include <gem.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN        4u
#define ARENA_BYTES  32768uL        /* well inside one 64 KB bank */
#define MAX_REQUEST  (60000uL)      /* one arena must hold it, and an arena is one bank at most */

typedef struct blk {
    struct blk *next;               /* address order; 0 ends the chain */
    unsigned long size;             /* payload bytes, a multiple of ALIGN */
    unsigned short used;
    unsigned short pad;             /* header stays a multiple of ALIGN: 12 bytes */
} blk;

static blk *g_head;                 /* first block of the first arena */
static blk *g_tail;                 /* the last arena's sentinel */

#define PAYLOAD(b)  ((void *)((char *)(b) + sizeof(blk)))
#define HEADER(p)   ((blk *)((char *)(p) - sizeof(blk)))
#define AFTER(b)    ((blk *)((char *)(b) + sizeof(blk) + (b)->size))

static unsigned long round_up(unsigned long n)
{
    return (n + (ALIGN - 1)) & ~(unsigned long)(ALIGN - 1);
}

/* Take a new arena from Malloc: one free block and the sentinel after it,
 * hung on the end of the chain.  0 if GEMDOS has no more. */
static blk *new_arena(unsigned long need)
{
    unsigned long bytes = ARENA_BYTES;
    char *base;
    blk *b, *s;

    if (need + 2 * sizeof(blk) > bytes)
        bytes = need + 2 * sizeof(blk);
    base = (char *)Malloc((LONG)bytes);
    if (base == 0)
        return 0;

    b = (blk *)base;
    b->size = bytes - 2 * sizeof(blk);
    b->used = 0;
    b->pad = 0;
    s = AFTER(b);
    s->size = 0;
    s->used = 1;
    s->pad = 0;
    s->next = 0;
    b->next = s;

    if (g_tail == 0)
        g_head = b;
    else
        g_tail->next = b;
    g_tail = s;
    return b;
}

/* Carve `need` bytes from free block b, leaving the rest free if there is
 * enough of it to be a block. */
static void *take(blk *b, unsigned long need)
{
    unsigned long rest = b->size - need;

    if (rest >= sizeof(blk) + ALIGN) {
        blk *r;
        b->size = need;
        r = AFTER(b);
        r->size = rest - sizeof(blk);
        r->used = 0;
        r->pad = 0;
        r->next = b->next;
        b->next = r;
    }
    b->used = 1;
    return PAYLOAD(b);
}

void *malloc(size_t n)
{
    unsigned long need = round_up((unsigned long)n);
    blk *b;

    if (need == 0)
        need = ALIGN;
    if (need > MAX_REQUEST)
        return 0;

    for (b = g_head; b != 0; b = b->next)
        if (!b->used && b->size >= need)
            return take(b, need);

    b = new_arena(need);
    if (b == 0)
        return 0;
    return take(b, need);
}

void free(void *p)
{
    blk *b, *prev, *q;

    if (p == 0)
        return;
    b = HEADER(p);
    b->used = 0;

    /* Merge with what follows, while it is free.  A sentinel is used, so
     * the merge never leaves the arena. */
    q = b->next;
    while (q != 0 && !q->used && q == AFTER(b)) {
        b->size += sizeof(blk) + q->size;
        b->next = q->next;
        q = b->next;
    }

    /* And with what precedes, if that is free and adjacent: a walk from
     * the head, which is what a singly linked chain costs. */
    prev = 0;
    for (q = g_head; q != 0 && q != b; q = q->next)
        prev = q;
    if (prev != 0 && !prev->used && AFTER(prev) == b) {
        prev->size += sizeof(blk) + b->size;
        prev->next = b->next;
    }
}

void *realloc(void *p, size_t n)
{
    blk *b;
    void *q;
    unsigned long need;

    if (p == 0)
        return malloc(n);
    if (n == 0) {
        free(p);
        return 0;
    }
    b = HEADER(p);
    need = round_up((unsigned long)n);
    if (b->size >= need)
        return p;                   /* it fits where it is */

    q = malloc(n);
    if (q == 0)
        return 0;
    memcpy(q, p, (size_t)b->size);
    free(p);
    return q;
}

void *calloc(size_t count, size_t size)
{
    unsigned long n = (unsigned long)count * (unsigned long)size;
    void *p;

    if (n > MAX_REQUEST)
        return 0;
    p = malloc((size_t)n);
    if (p != 0)
        memset(p, 0, (size_t)n);
    return p;
}
