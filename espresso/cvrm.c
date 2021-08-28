/*
    module: cvrm.c
    Purpose: miscellaneous cover manipulation
        a) verify two covers are equal, check consistency of a cover
        b) unravel a multiple-valued cover into minterms
        c) sort covers
*/

#include "espresso.h"

static void cb_unravel(pcube c, pcube startbase, pcover B1) {
    pcube base = cube.temp[0], p, last;
    int expansion, place, size, offset;
    int i, j, n;

    /* Determine how many cubes it will blow up into, and create a mask
        for those parts that have only a single coordinate
    */
    expansion = 1;
    (void)set_copy(base, startbase);
    if ((size = set_dist(c, cube.var_mask[cube.output])) < 2) {
        (void)set_or(base, base, cube.var_mask[cube.output]);
    } else {
        expansion *= size;
    }
    (void)set_and(base, c, base);

    /* Add the unravelled sets starting at the last element of B1 */
    offset = B1->count;
    B1->count += expansion;
    foreach_remaining_set(B1, last, GETSET(B1, offset - 1), p) {
        INLINEset_copy(p, base);
    }

    if ((size = set_dist(c, cube.var_mask[cube.output])) > 1) {
        place = expansion / size;
        n = 0;
        for (i = cube.first_part[cube.output]; i <= cube.last_part[cube.output];
             i++) {
            if (is_in_set(c, i)) {
                for (j = 0; j < place; j++) {
                    p = GETSET(B1, n + j + offset);
                    (void)set_insert(p, i);
                }
                n += place;
            }
        }
    }
}

/**
 * Unravel on output part
 * @param B Cover to be unraveled, can have multiple `1`s in the output part of
 *              every cube.
 * @return A cover which has the output part of every cube 'one-hot'.
 */
pcover unravel_output(pcover B) {
    pcover B1;
    int var, total_size, expansion, size;
    pcube p, last, startbase = cube.temp[1];

    /* Create the starting base for those variables not being unravelled */
    (void)set_copy(startbase, cube.emptyset);
    for (var = 0; var < cube.output; var++)
        (void)set_or(startbase, startbase, cube.var_mask[var]);

    /* Determine how many cubes it will blow up into */
    total_size = 0;
    foreach_set(B, last, p) {
        expansion = 1;
        if ((size = set_dist(p, cube.var_mask[cube.output])) >= 2)
            if ((expansion *= size) > 1000000)
                fatal("unreasonable expansion in unravel");
        total_size += expansion;
    }

    /* We can now allocate a cover of exactly the correct size */
    B1 = new_cover(total_size);
    foreach_set(B, last, p) {
        cb_unravel(p, startbase, B1);
    }
    free_cover(B);
    return B1;
}

/*  mini_sort -- sort cubes according to the heuristics of mini */
pcover mini_sort(pcover F, int (*compare)(pset *, pset *)) {
    int *count, cnt, n = cube.size, i;
    pcube p, last;
    pcover F_sorted;
    pcube *F1;

    /* Perform a column sum over the set family */
    count = sf_count(F);

    /* weight is "inner product of the cube and the column sums" */
    foreach_set(F, last, p) {
        cnt = 0;
        for (i = 0; i < n; i++)
            if (is_in_set(p, i))
                cnt += count[i];
        PUTSIZE(p, cnt);
    }
    FREE(count);

    /* use qsort to sort the array */
    qsort((char *)(F1 = sf_list(F)), F->count, sizeof(pcube),
          (int (*)(const void *, const void *))compare);
    F_sorted = sf_unlist(F1, F->count, F->sf_size);
    free_cover(F);

    return F_sorted;
}

/* sort_reduce -- Espresso strategy for ordering the cubes before reduction */
pcover sort_reduce(pcover T) {
    pcube p, last, largest = NULL;
    int bestsize = -1, size, n = cube.num_input_vars + 1;
    pcover T_sorted;
    pcube *T1;

    if (T->count == 0)
        return T;

    /* find largest cube */
    foreach_set(T, last, p) if ((size = set_ord(p)) > bestsize) largest = p,
                                                                bestsize = size;

    foreach_set(T, last, p)
        PUTSIZE(p, ((n - cdist(largest, p)) << 7) + MIN(set_ord(p), 127));

    qsort((char *)(T1 = sf_list(T)), T->count, sizeof(pcube),
          (int (*)(const void *, const void *))descend);
    T_sorted = sf_unlist(T1, T->count, T->sf_size);
    free_cover(T);

    return T_sorted;
}

/*
 *  cubelist_partition -- take a cubelist T and see if it has any components;
 *  if so, return cubelist's of the two partitions A and B; the return value
 *  is the size of the partition; if not, A and B
 *  are undefined and the return value is 0
 */
int cubelist_partition(pcube *T, /* a list of cubes */
                       pcube **A,
                       pcube **B /* cubelist of partition and remainder */) {
    pcube *T1, p, seed, cof;
    pcube *A1, *B1;
    bool change;
    int count, numcube;

    numcube = CUBELISTSIZE(T);

    /* Mark all cubes -- covered cubes belong to the partition */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        RESET(p, COVERED);
    }

    /*
     *  Extract a partition from the cubelist T; start with the first cube as a
     *  seed, and then pull in all cubes which share a variable with the seed;
     *  iterate until no new cubes are brought into the partition.
     */
    seed = set_save(T[2]);
    cof = T[0];
    SET(T[2], COVERED);
    count = 1;

    do {
        change = FALSE;
        for (T1 = T + 2; (p = *T1++) != NULL;) {
            if (!TESTP(p, COVERED) && ccommon(p, seed, cof)) {
                INLINEset_and(seed, seed, p);
                SET(p, COVERED);
                change = TRUE;
                count++;
            }
        }
    } while (change);

    set_free(seed);

    if (count != numcube) {
        /* Allocate and setup the cubelist's for the two partitions */
        *A = A1 = ALLOC(pcube, numcube + 3);
        *B = B1 = ALLOC(pcube, numcube + 3);
        (*A)[0] = set_save(T[0]);
        (*B)[0] = set_save(T[0]);
        A1 = *A + 2;
        B1 = *B + 2;

        /* Loop over the cubes in T and distribute to A and B */
        for (T1 = T + 2; (p = *T1++) != NULL;) {
            if (TESTP(p, COVERED)) {
                *A1++ = p;
            } else {
                *B1++ = p;
            }
        }

        /* Stuff needed at the end of the cubelist's */
        *A1++ = NULL;
        (*A)[1] = (pcube)A1;
        *B1++ = NULL;
        (*B)[1] = (pcube)B1;
    }

    return numcube - count;
}
