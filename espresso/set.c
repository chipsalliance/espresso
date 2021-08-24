/*
 *   set.c -- routines for manipulating sets and set families
 */

#include "espresso.h"
static pset_family set_family_garbage = NULL;

static void intcpy(unsigned int *d, unsigned int *s, long n) {
    int i;
    for (i = 0; i < n; i++) {
        *d++ = *s++;
    }
}

/* bit_index -- find first bit (from LSB) in a word (MSB=bit n, LSB=bit 0) */
int bit_index(unsigned int a) {
    int i;
    if (a == 0)
        return -1;
    for (i = 0; (a & 1) == 0; a >>= 1, i++)
        ;
    return i;
}

/* set_ord -- count number of elements in a set */
int set_ord(pset a) {
    int i, sum = 0;
    unsigned int val;
    for (i = LOOP(a); i > 0; i--)
        if ((val = a[i]) != 0)
            sum += count_ones(val);
    return sum;
}

/* set_dist -- distance between two sets (# elements in common) */
int set_dist(pset a, pset b) {
    int i, sum = 0;
    unsigned int val;
    for (i = LOOP(a); i > 0; i--)
        if ((val = a[i] & b[i]) != 0)
            sum += count_ones(val);
    return sum;
}

/* set_clear -- make "r" the empty set of "size" elements */
pset set_clear(pset r, int size) {
    int i = LOOPINIT(size);
    *r = i;
    do
        r[i] = 0;
    while (--i > 0);
    return r;
}

/* set_fill -- make "r" the universal set of "size" elements */
pset set_fill(pset r, int size) {
    int i = LOOPINIT(size);
    *r = i;
    r[i] = ~(unsigned)0;
    r[i] >>= i * BPI - size;
    while (--i > 0)
        r[i] = ~(unsigned)0;
    return r;
}

/* set_copy -- copy set a into set r */
pset set_copy(pset r, pset a) {
    int i = LOOPCOPY(a);
    do
        r[i] = a[i];
    while (--i >= 0);
    return r;
}

/* set_and -- compute intersection of sets "a" and "b" */
pset set_and(pset r, pset a, pset b) {
    int i = LOOP(a);
    PUTLOOP(r, i);
    do
        r[i] = a[i] & b[i];
    while (--i > 0);
    return r;
}

/* set_or -- compute union of sets "a" and "b" */
pset set_or(pset r, pset a, pset b) {
    int i = LOOP(a);
    PUTLOOP(r, i);
    do
        r[i] = a[i] | b[i];
    while (--i > 0);
    return r;
}

/* set_diff -- compute difference of sets "a" and "b" */
pset set_diff(pset r, pset a, pset b) {
    int i = LOOP(a);
    PUTLOOP(r, i);
    do
        r[i] = a[i] & ~b[i];
    while (--i > 0);
    return r;
}

/* set_merge -- compute "a" & "mask" | "b" & ~ "mask" */
pset set_merge(pset r, pset a, pset b, pset mask) {
    int i = LOOP(a);
    PUTLOOP(r, i);
    do
        r[i] = (a[i] & mask[i]) | (b[i] & ~mask[i]);
    while (--i > 0);
    return r;
}

/* setp_empty -- check if the set "a" is empty */
bool setp_empty(pset a) {
    int i = LOOP(a);
    do
        if (a[i])
            return FALSE;
    while (--i > 0);
    return TRUE;
}

/* setp_equal -- check if the set "a" equals set "b" */
bool setp_equal(pset a, pset b) {
    int i = LOOP(a);
    do
        if (a[i] != b[i])
            return FALSE;
    while (--i > 0);
    return TRUE;
}

/* setp_disjoint -- check if intersection of "a" and "b" is empty */
bool setp_disjoint(pset a, pset b) {
    int i = LOOP(a);
    do
        if (a[i] & b[i])
            return FALSE;
    while (--i > 0);
    return TRUE;
}

/* setp_implies -- check if "a" implies "b" ("b" contains "a") */
bool setp_implies(pset a, pset b) {
    int i = LOOP(a);
    do
        if (a[i] & ~b[i])
            return FALSE;
    while (--i > 0);
    return TRUE;
}

/* sf_active -- make all members of the set family active */
pset_family sf_active(pset_family A) {
    pset p, last;
    foreach_set(A, last, p) {
        SET(p, ACTIVE);
    }
    A->active_count = A->count;
    return A;
}

/* sf_inactive -- remove all inactive cubes in a set family */
pset_family sf_inactive(pset_family A) {
    pset p, last, pdest;

    pdest = A->data;
    foreach_set(A, last, p) {
        if (TESTP(p, ACTIVE)) {
            if (pdest != p) {
                INLINEset_copy(pdest, p);
            }
            pdest += A->wsize;
        } else {
            A->count--;
        }
    }
    return A;
}

/* sf_copy -- copy a set family */
pset_family sf_copy(pset_family R, pset_family A) {
    R->sf_size = A->sf_size;
    R->wsize = A->wsize;
    /*R->capacity = A->count;*/
    /*R->data = REALLOC(unsigned int, R->data, (long) R->capacity * R->wsize);*/
    R->count = A->count;
    R->active_count = A->active_count;
    intcpy(R->data, A->data, (long)A->wsize * A->count);
    return R;
}

/* sf_join -- join A and B into a single set_family */
pset_family sf_join(pset_family A, pset_family B) {
    pset_family R;
    long asize = A->count * A->wsize;
    long bsize = B->count * B->wsize;

    if (A->sf_size != B->sf_size)
        fatal("sf_join: sf_size mismatch");
    R = sf_new(A->count + B->count, A->sf_size);
    R->count = A->count + B->count;
    R->active_count = A->active_count + B->active_count;
    intcpy(R->data, A->data, asize);
    intcpy(R->data + asize, B->data, bsize);
    return R;
}

/* sf_append -- append the sets of B to the end of A, and dispose of B */
pset_family sf_append(pset_family A, pset_family B) {
    long asize = A->count * A->wsize;
    long bsize = B->count * B->wsize;

    if (A->sf_size != B->sf_size)
        fatal("sf_append: sf_size mismatch");
    A->capacity = A->count + B->count;
    A->data = REALLOC(unsigned int, A->data, (long)A->capacity * A->wsize);
    intcpy(A->data + asize, B->data, bsize);
    A->count += B->count;
    A->active_count += B->active_count;
    sf_free(B);
    return A;
}

/* sf_new -- allocate "num" sets of "size" elements each */
pset_family sf_new(int num, int size) {
    pset_family A;
    if (set_family_garbage == NULL) {
        A = ALLOC(set_family_t, 1);
    } else {
        A = set_family_garbage;
        set_family_garbage = A->next;
    }
    A->sf_size = size;
    A->wsize = SET_SIZE(size);
    A->capacity = num;
    A->data = ALLOC(unsigned int, (long)A->capacity * A->wsize);
    A->count = 0;
    A->active_count = 0;
    return A;
}

/* sf_save -- create a duplicate copy of a set family */
pset_family sf_save(pset_family A) {
    return sf_copy(sf_new(A->count, A->sf_size), A);
}

/* sf_free -- free the storage allocated for a set family */
void sf_free(pset_family A) {
    FREE(A->data);
    A->next = set_family_garbage;
    set_family_garbage = A;
}

/* sf_cleanup -- free all of the set families from the garbage list */
void sf_cleanup() {
    pset_family p, pnext;
    for (p = set_family_garbage; p != (pset_family)NULL; p = pnext) {
        pnext = p->next;
        FREE(p);
    }
    set_family_garbage = (pset_family)NULL;
}

/* sf_addset -- add a set to the end of a set family */
pset_family sf_addset(pset_family A, pset s) {
    pset p;

    if (A->count >= A->capacity) {
        A->capacity = A->capacity + A->capacity / 2 + 1;
        A->data = REALLOC(unsigned int, A->data, (long)A->capacity * A->wsize);
    }
    p = GETSET(A, A->count++);
    INLINEset_copy(p, s);
    return A;
}

/* set_adjcnt -- adjust the counts for a set by "weight" */
void set_adjcnt(pset a, int *count, int weight) {
    int i, base;
    unsigned int val;

    for (i = LOOP(a); i > 0;) {
        for (val = a[i], base = --i << LOGBPI; val != 0; base++, val >>= 1) {
            if (val & 1) {
                count[base] += weight;
            }
        }
    }
}

/* sf_count -- perform a column sum over a set family */
int *sf_count(pset_family A) {
    pset p, last;
    int i, base, *count;
    unsigned int val;

    count = ALLOC(int, A->sf_size);
    for (i = A->sf_size - 1; i >= 0; i--) {
        count[i] = 0;
    }

    foreach_set(A, last, p) {
        for (i = LOOP(p); i > 0;) {
            for (val = p[i], base = --i << LOGBPI; val != 0;
                 base++, val >>= 1) {
                if (val & 1) {
                    count[base]++;
                }
            }
        }
    }
    return count;
}

/* sf_count_restricted -- perform a column sum over a set family, restricting
 * to only the columns which are in r; also, the columns are weighted by the
 * number of elements which are in each row
 */
int *sf_count_restricted(pset_family A, pset r) {
    pset p;
    int i, base, *count;
    unsigned int val;
    int weight;
    pset last;

    count = ALLOC(int, A->sf_size);
    for (i = A->sf_size - 1; i >= 0; i--) {
        count[i] = 0;
    }

    /* Loop for each set */
    foreach_set(A, last, p) {
        weight = 1024 / (set_ord(p) - 1);
        for (i = LOOP(p); i > 0;) {
            for (val = p[i] & r[i], base = --i << LOGBPI; val != 0;
                 base++, val >>= 1) {
                if (val & 1) {
                    count[base] += weight;
                }
            }
        }
    }
    return count;
}
