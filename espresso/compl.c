/*
 *  module: compl.c
 *  purpose: compute the complement of a multiple-valued function
 *
 *  The "unate recursive paradigm" is used.  After a set of special
 *  cases are examined, the function is split on the "most active
 *  variable".  These two halves are complemented recursively, and then
 *  the results are merged.
 *
 *  Changes (from Version 2.1 to Version 2.2)
 *      1. Minor bug in compl_lifting -- cubes in the left half were
 *      not marked as active, so that when merging a leaf from the left
 *      hand side, the active flags were essentially random.  This led
 *      to minor impreictability problem, but never affected the
 *      accuracy of the results.
 */

#include "espresso.h"

#define USE_COMPL_LIFT       0
#define USE_COMPL_LIFT_ONSET 1

/* compl_cube -- return the complement of a single cube (De Morgan's law) */
static pcover compl_cube(pcube p) {
    pcube diff = cube.temp[7], pdest, mask, full = cube.fullset;
    int var;
    pcover R;

    /* Allocate worst-case size cover (to avoid checking overflow) */
    R = new_cover(cube.num_input_vars + 1);

    /* Compute bit-wise complement of the cube */
    INLINEset_diff(diff, full, p);

    for (var = 0; var < cube.num_input_vars + 1; var++) {
        mask = cube.var_mask[var];
        /* If the bit-wise complement is not empty in var ... */
        if (!setp_disjoint(diff, mask)) {
            pdest = GETSET(R, R->count++);
            INLINEset_merge(pdest, diff, full, mask);
        }
    }
    return R;
}

static bool compl_special_cases(
    pcube *T,    /* will be disposed if answer is determined */
    pcover *Tbar /* returned only if answer determined */
) {
    pcube *T1, p, ceil, cof = T[0];
    pcover A, ceil_compl;

    /* Check for no cubes in the cover */
    if (T[2] == NULL) {
        *Tbar = sf_addset(new_cover(1), cube.fullset);
        free_cubelist(T);
        return TRUE;
    }

    /* Check for only a single cube in the cover */
    if (T[3] == NULL) {
        *Tbar = compl_cube(set_or(cof, cof, T[2]));
        free_cubelist(T);
        return TRUE;
    }

    /* Check for a row of all 1's (implies complement is null) */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        if (full_row(p, cof)) {
            *Tbar = new_cover(0);
            free_cubelist(T);
            return TRUE;
        }
    }

    /* Check for a column of all 0's which can be factored out */
    ceil = set_save(cof);
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        INLINEset_or(ceil, ceil, p);
    }
    if (!setp_equal(ceil, cube.fullset)) {
        ceil_compl = compl_cube(ceil);
        (void)set_or(cof, cof, set_diff(ceil, cube.fullset, ceil));
        set_free(ceil);
        *Tbar = sf_append(complement(T), ceil_compl);
        return TRUE;
    }
    set_free(ceil);

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If single active variable not factored out above, then tautology ! */
    if (cdata.vars_active == 1) {
        *Tbar = new_cover(0);
        free_cubelist(T);
        return TRUE;

        /* Check for unate cover */
    } else if (cdata.vars_unate == cdata.vars_active) {
        A = map_cover_to_unate(T);
        free_cubelist(T);
        A = unate_compl(A);
        *Tbar = map_unate_to_cover(A);
        sf_free(A);
        return TRUE;

        /* Not much we can do about it */
    } else {
        return MAYBE;
    }
}

/*
 *  compl_d1merge -- distance-1 merge in the splitting variable
 */
static void compl_d1merge(pcube *L1, pcube *R1) {
    pcube pl, pr;

    /* Find equal cubes between the two cofactors */
    for (pl = *L1, pr = *R1; (pl != NULL) && (pr != NULL);)
        switch (d1_order(L1, R1)) {
            case 1:
                pr = *(++R1);
                break; /* advance right pointer */
            case -1:
                pl = *(++L1);
                break; /* advance left pointer */
            case 0:
                RESET(pr, ACTIVE);
                INLINEset_or(pl, pl, pr);
                pr = *(++R1);
        }
}

/*
 *  compl_lift_onset -- expand in the splitting variable using a
 *  distance-1 check against the original on-set; expand all (or
 *  none) of the splitting variable.  Each cube of A1 is expanded
 *  against the original on-set T.
 */
static void compl_lift_onset(pcube *A1, pcover T, pcube bcube, int var) {
    pcube a, last, p, lift = cube.temp[4], mask = cube.var_mask[var];

    /* for each active cube from one branch of the complement */
    for (; (a = *A1++) != NULL;) {
        if (TESTP(a, ACTIVE)) {
            /* create a lift of this cube in the merging coord */
            INLINEset_and(lift, bcube, mask); /* isolate parts to raise */
            INLINEset_or(lift, a, lift);      /* raise these parts in a */

            /* for each cube in the ON-set, check for intersection */
            foreach_set(T, last, p) {
                if (cdist0(p, lift)) {
                    goto nolift;
                }
            }
            INLINEset_copy(a, lift); /* save the raising */
            SET(a, ACTIVE);
        nolift:;
        }
    }
}

/*
 *  compl_lift_simple -- expand in the splitting variable using single
 *  cube containment against the other recursion branch to check
 *  validity of the expansion, and expanding all (or none) of the
 *  splitting variable.
 */
static void compl_lift(pcube *A1, pcube *B1, pcube bcube, int var) {
    pcube a, b, *B2, lift = cube.temp[4], liftor = cube.temp[5];
    pcube mask = cube.var_mask[var];

    (void)set_and(liftor, bcube, mask);

    /* for each cube in the first array ... */
    for (; (a = *A1++) != NULL;) {
        if (TESTP(a, ACTIVE)) {
            /* create a lift of this cube in the merging coord */
            (void)set_merge(lift, bcube, a, mask);

            /* for each cube in the second array */
            for (B2 = B1; (b = *B2++) != NULL;) {
                INLINEsetp_implies(lift, b, /* when_false => */ continue);
                /* when_true => fall through to next statement */

                /* cube of A1 was contained by some cube of B1, so raise */
                INLINEset_or(a, a, liftor);
                break;
            }
        }
    }
}

/*
 *  compl_merge -- merge the two cofactors around the splitting
 *  variable
 *
 *  The merge operation involves intersecting each cube of the left
 *  cofactor with cl, and intersecting each cube of the right cofactor
 *  with cr.  The union of these two covers is the merged result.
 *
 *  In order to reduce the number of cubes, a distance-1 merge is
 *  performed (note that two cubes can only combine distance-1 in the
 *  splitting variable).  Also, a simple expand is performed in the
 *  splitting variable (simple implies the covering check for the
 *  expansion is not full containment, but single-cube containment).
 */
static pcover compl_merge(pcube *T1, /* Original ON-set */
                          pcover L,
                          pcover R, /* Complement from each recursion branch */
                          pcube cl, pcube cr, /* cubes used for cofactoring */
                          int var,            /* splitting variable */
                          int lifting /* whether to perform lifting or not */
) {
    pcube p, last, pt;
    pcover T, Tbar;
    pcube *L1, *R1;

    /* Intersect each cube with the cofactored cube */
    foreach_set(L, last, p) {
        INLINEset_and(p, p, cl);
        SET(p, ACTIVE);
    }
    foreach_set(R, last, p) {
        INLINEset_and(p, p, cr);
        SET(p, ACTIVE);
    }

    /* Sort the arrays for a distance-1 merge */
    (void)set_copy(cube.temp[0], cube.var_mask[var]);
    qsort((char *)(L1 = sf_list(L)), L->count, sizeof(pset),
          (int (*)(const void *, const void *))d1_order);
    qsort((char *)(R1 = sf_list(R)), R->count, sizeof(pset),
          (int (*)(const void *, const void *))d1_order);

    /* Perform distance-1 merge */
    compl_d1merge(L1, R1);

    /* Perform lifting */
    switch (lifting) {
        case USE_COMPL_LIFT_ONSET:
            T = cubeunlist(T1);
            compl_lift_onset(L1, T, cr, var);
            compl_lift_onset(R1, T, cl, var);
            free_cover(T);
            break;
        case USE_COMPL_LIFT:
            compl_lift(L1, R1, cr, var);
            compl_lift(R1, L1, cl, var);
            break;
    }
    FREE(L1);
    FREE(R1);

    /* Re-create the merged cover */
    Tbar = new_cover(L->count + R->count);
    pt = Tbar->data;
    foreach_set(L, last, p) {
        INLINEset_copy(pt, p);
        Tbar->count++;
        pt += Tbar->wsize;
    }
    foreach_active_set(R, last, p) {
        INLINEset_copy(pt, p);
        Tbar->count++;
        pt += Tbar->wsize;
    }

    free_cover(L);
    free_cover(R);
    return Tbar;
}

/* complement -- compute the complement of T */
pcover complement(pcube *T /* T will be disposed of */
) {
    pcube cl, cr;
    int best;
    pcover Tbar, Tl, Tr;
    int lifting;

    if (compl_special_cases(T, &Tbar) == MAYBE) {
        /* Allocate space for the partition cubes */
        cl = new_cube();
        cr = new_cube();
        best = binate_split_select(T, cl, cr);

        /* Complement the left and right halves */
        Tl = complement(scofactor(T, cl, best));
        Tr = complement(scofactor(T, cr, best));

        if (Tr->count * Tl->count > (Tr->count + Tl->count) * CUBELISTSIZE(T)) {
            lifting = USE_COMPL_LIFT_ONSET;
        } else {
            lifting = USE_COMPL_LIFT;
        }
        Tbar = compl_merge(T, Tl, Tr, cl, cr, best, lifting);

        free_cube(cl);
        free_cube(cr);
        free_cubelist(T);
    }

    return Tbar;
}
