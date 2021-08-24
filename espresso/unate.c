/*
 *  unate.c -- routines for dealing with unate functions
 */

#include "espresso.h"

pcover map_cover_to_unate(pcube *T) {
    unsigned int word_test, word_set, bit_test, bit_set;
    pcube p, pA;
    pset_family A;
    pcube *T1;
    int ncol, i;

    A = sf_new(CUBELISTSIZE(T), cdata.vars_unate);
    A->count = CUBELISTSIZE(T);
    foreachi_set(A, i, p) {
        (void)set_clear(p, A->sf_size);
    }
    ncol = 0;

    for (i = 0; i < cube.size; i++) {
        if (cdata.part_zeros[i] > 0) {
            assert(ncol <= cdata.vars_unate);

            /* Copy a column from T to A */
            word_test = WHICH_WORD(i);
            bit_test = 1 << WHICH_BIT(i);
            word_set = WHICH_WORD(ncol);
            bit_set = 1 << WHICH_BIT(ncol);

            pA = A->data;
            for (T1 = T + 2; (p = *T1++) != 0;) {
                if ((p[word_test] & bit_test) == 0) {
                    pA[word_set] |= bit_set;
                }
                pA += A->wsize;
            }

            ncol++;
        }
    }

    return A;
}

pcover map_unate_to_cover(pset_family A) {
    int i, ncol, lp;
    pcube p, pB;
    int var, nunate, *unate;
    pcube last;
    pset_family B;

    B = sf_new(A->count, cube.size);
    B->count = A->count;

    /* Find the unate variables */
    unate = ALLOC(int, cube.num_vars);
    nunate = 0;
    for (var = 0; var < cube.num_vars; var++) {
        if (cdata.is_unate[var]) {
            unate[nunate++] = var;
        }
    }

    /* Loop for each set of A */
    pB = B->data;
    foreach_set(A, last, p) {
        /* Initialize this set of B */
        INLINEset_fill(pB, cube.size);

        /* Now loop for the unate variables; if the part is in A,
         * then this variable of B should be a single 1 in the unate
         * part.
         */
        for (ncol = 0; ncol < nunate; ncol++) {
            if (is_in_set(p, ncol)) {
                lp = cube.last_part[unate[ncol]];
                for (i = cube.first_part[unate[ncol]]; i <= lp; i++) {
                    if (cdata.part_zeros[i] == 0) {
                        set_remove(pB, i);
                    }
                }
            }
        }
        pB += B->wsize;
    }

    FREE(unate);
    return B;
}

/*
 *  unate_compl
 */

pset_family unate_compl(pset_family A) {
    pset p, last;

    /* Make sure A is single-cube containment minimal */
    /*    A = sf_rev_contain(A);*/

    foreach_set(A, last, p) {
        PUTSIZE(p, set_ord(p));
    }

    /* Recursively find the complement */
    A = unate_complement(A);

    /* Now, we can guarantee a minimal result by containing the result */
    A = sf_rev_contain(A);
    return A;
}

/*
 *  abs_covered -- after selecting a new column for the selected set,
 *  create a new matrix which is only those rows which are still uncovered
 */
static pset_family abs_covered(pset_family A, int pick) {
    pset last, p, pdest;
    pset_family Aprime;

    Aprime = sf_new(A->count, A->sf_size);
    pdest = Aprime->data;
    foreach_set(A, last, p) if (!is_in_set(p, pick)) {
        INLINEset_copy(pdest, p);
        Aprime->count++;
        pdest += Aprime->wsize;
    }
    return Aprime;
}

/*
 *  abs_covered_many -- after selecting many columns for ther selected set,
 *  create a new matrix which is only those rows which are still uncovered
 */
static pset_family abs_covered_many(pset_family A, pset pick_set) {
    pset last, p, pdest;
    pset_family Aprime;

    Aprime = sf_new(A->count, A->sf_size);
    pdest = Aprime->data;
    foreach_set(A, last, p) if (setp_disjoint(p, pick_set)) {
        INLINEset_copy(pdest, p);
        Aprime->count++;
        pdest += Aprime->wsize;
    }
    return Aprime;
}

/*
 *  abs_select_restricted -- select the column of maximum column count which
 *  also belongs to the set "restrict"; weight each column of a set as
 *  1 / (set_ord(p) - 1).
 */
static int abs_select_restricted(pset_family A, pset _restrict) {
    int i, best_var, best_count, *count;

    /* Sum the elements in these columns */
    count = sf_count_restricted(A, _restrict);

    /* Find which variable has maximum weight */
    best_var = -1;
    best_count = 0;
    for (i = 0; i < A->sf_size; i++) {
        if (count[i] > best_count) {
            best_var = i;
            best_count = count[i];
        }
    }
    FREE(count);

    if (best_var == -1)
        fatal("abs_select_restricted: should not have best_var == -1");

    return best_var;
}

/*
 *  Assume SIZE(p) records the size of each set
 */
pset_family unate_complement(pset_family A /* disposes of A */
) {
    pset_family Abar;
    pset p, p1, _restrict;
    int i;
    int max_i, min_set_ord, j;

    /* Check for no sets in the matrix -- complement is the universe */
    if (A->count == 0) {
        sf_free(A);
        Abar = sf_new(1, A->sf_size);
        (void)set_clear(GETSET(Abar, Abar->count++), A->sf_size);

        /* Check for a single set in the maxtrix -- compute de Morgan complement
         */
    } else if (A->count == 1) {
        p = A->data;
        Abar = sf_new(A->sf_size, A->sf_size);
        for (i = 0; i < A->sf_size; i++) {
            if (is_in_set(p, i)) {
                p1 = set_clear(GETSET(Abar, Abar->count++), A->sf_size);
                set_insert(p1, i);
            }
        }
        sf_free(A);

    } else {
        /* Select splitting variable as the variable which belongs to a set
         * of the smallest size, and which has greatest column count
         */
        _restrict = set_new(A->sf_size);
        min_set_ord = A->sf_size + 1;
        foreachi_set(A, i, p) {
            if (SIZE(p) < min_set_ord) {
                set_copy(_restrict, p);
                min_set_ord = SIZE(p);
            } else if (SIZE(p) == min_set_ord) {
                set_or(_restrict, _restrict, p);
            }
        }

        /* Check for no data (shouldn't happen ?) */
        if (min_set_ord == 0) {
            A->count = 0;
            Abar = A;

            /* Check for "essential" columns */
        } else if (min_set_ord == 1) {
            Abar = unate_complement(abs_covered_many(A, _restrict));
            sf_free(A);
            foreachi_set(Abar, i, p) {
                set_or(p, p, _restrict);
            }

            /* else, recur as usual */
        } else {
            max_i = abs_select_restricted(A, _restrict);

            /* Select those rows of A which are not covered by max_i,
             * recursively find all minimal covers of these rows, and
             * then add back in max_i
             */
            Abar = unate_complement(abs_covered(A, max_i));
            foreachi_set(Abar, i, p) {
                set_insert(p, max_i);
            }

            /* Now recur on A with all zero's on column max_i */
            foreachi_set(A, i, p) {
                if (is_in_set(p, max_i)) {
                    set_remove(p, max_i);
                    j = SIZE(p) - 1;
                    PUTSIZE(p, j);
                }
            }

            Abar = sf_append(Abar, unate_complement(A));
        }
        set_free(_restrict);
    }

    return Abar;
}
