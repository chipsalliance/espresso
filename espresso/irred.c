#include "espresso.h"

static int Rp_current;

/*
 *   irredundant -- Return a minimal subset of F
 */
pcover irredundant(pcover F, pcover D) {
    mark_irredundant(F, D);
    return sf_inactive(F);
}

/*
 *   mark_irredundant -- find redundant cubes, and mark them "INACTIVE"
 */
void mark_irredundant(pcover F, pcover D) {
    pcover E, Rt, Rp;
    pset p, p1, last;
    sm_matrix *table;
    sm_row *cover;
    sm_element *pe;

    /* extract a minimum cover */
    irred_split_cover(F, D, &E, &Rt, &Rp);
    table = irred_derive_table(D, E, Rp);
    cover = sm_minimum_cover(table, NIL(int), /* heuristic */ 1);

    /* mark the cubes for the result */
    foreach_set(F, last, p) {
        RESET(p, ACTIVE);
        RESET(p, RELESSEN);
    }
    foreach_set(E, last, p) {
        p1 = GETSET(F, SIZE(p));
        assert(setp_equal(p1, p));
        SET(p1, ACTIVE);
        SET(p1, RELESSEN); /* for essen(), mark as rel. ess. */
    }
    sm_foreach_row_element(cover, pe) {
        p1 = GETSET(F, pe->col_num);
        SET(p1, ACTIVE);
    }

    free_cover(E);
    free_cover(Rt);
    free_cover(Rp);
    sm_free(table);
    sm_row_free(cover);
}

/*
 *  irred_split_cover -- find E, Rt, and Rp from the cover F, D
 *
 *	E  -- relatively essential cubes
 *	Rt  -- totally redundant cubes
 *	Rp  -- partially redundant cubes
 */
void irred_split_cover(pcover F, pcover D, pcover *E, pcover *Rt, pcover *Rp) {
    pcube p, last;
    int index;
    pcover R;
    pcube *FD, *ED;

    /* number the cubes of F -- these numbers track into E, Rp, Rt, etc. */
    index = 0;
    foreach_set(F, last, p) {
        PUTSIZE(p, index);
        index++;
    }

    *E = new_cover(10);
    *Rt = new_cover(10);
    *Rp = new_cover(10);
    R = new_cover(10);

    /* Split F into E and R */
    FD = cube2list(F, D);
    foreach_set(F, last, p) {
        if (cube_is_covered(FD, p)) {
            R = sf_addset(R, p);
        } else {
            *E = sf_addset(*E, p);
        }
    }
    free_cubelist(FD);

    /* Split R into Rt and Rp */
    ED = cube2list(*E, D);
    foreach_set(R, last, p) {
        if (cube_is_covered(ED, p)) {
            *Rt = sf_addset(*Rt, p);
        } else {
            *Rp = sf_addset(*Rp, p);
        }
    }
    free_cubelist(ED);

    free_cover(R);
}

static bool ftaut_special_cases(
    pcube *T, /* will be disposed if answer is determined */
    sm_matrix *table) {
    pcube *T1, *Tsave, p, temp = cube.temp[0], ceil = cube.temp[1];
    int var, rownum;

    /* Check for a row of all 1's in the essential cubes */
    for (T1 = T + 2; (p = *T1++) != 0;) {
        if (!TESTP(p, REDUND)) {
            if (full_row(p, T[0])) {
                /* subspace is covered by essentials -- no new rows for table */
                free_cubelist(T);
                return TRUE;
            }
        }
    }

/* Collect column counts, determine unate variables, etc. */
start:
    massive_count(T);

    /* If function is unate, find the rows of all 1's */
    if (cdata.vars_unate == cdata.vars_active) {
        /* find which nonessentials cover this subspace */
        rownum = table->last_row ? table->last_row->row_num + 1 : 0;
        (void)sm_insert(table, rownum, Rp_current);
        for (T1 = T + 2; (p = *T1++) != 0;) {
            if (TESTP(p, REDUND)) {
                /* See if a redundant cube covers this leaf */
                if (full_row(p, T[0])) {
                    (void)sm_insert(table, rownum, (int)SIZE(p));
                }
            }
        }
        free_cubelist(T);
        return TRUE;

        /* Perform unate reduction if there are any unate variables */
    } else if (cdata.vars_unate != 0) {
        /* Form a cube "ceil" with full variables in the unate variables */
        (void)set_copy(ceil, cube.emptyset);
        for (var = 0; var < cube.num_input_vars + 1; var++) {
            if (cdata.is_unate[var]) {
                INLINEset_or(ceil, ceil, cube.var_mask[var]);
            }
        }

        /* Save only those cubes that are "full" in all unate variables */
        for (Tsave = T1 = T + 2; (p = *T1++) != 0;) {
            if (setp_implies(ceil, set_or(temp, p, T[0]))) {
                *Tsave++ = p;
            }
        }
        *Tsave++ = 0;
        T[1] = (pcube)Tsave;
        goto start;
    }

    /* Not much we can do about it */
    return MAYBE;
}

/* ftautology -- find ways to make a tautology */
static void ftautology(pcube *T, /* T will be disposed of */
                       sm_matrix *table) {
    pcube cl, cr;
    int best;

    if (ftaut_special_cases(T, table) == MAYBE) {
        cl = new_cube();
        cr = new_cube();
        best = binate_split_select(T, cl, cr);

        ftautology(scofactor(T, cl, best), table);
        ftautology(scofactor(T, cr, best), table);

        free_cubelist(T);
        free_cube(cl);
        free_cube(cr);
    }
}

/* fcube_is_covered -- determine exactly how a cubelist "covers" a cube */
static void fcube_is_covered(pcube *T, pcube c, sm_matrix *table) {
    ftautology(cofactor(T, c), table);
}

/*
 *  irred_derive_table -- given the covers D, E and the set of
 *  partially redundant primes Rp, build a covering table showing
 *  possible selections of primes to cover Rp.
 */
sm_matrix *irred_derive_table(pcover D, pcover E, pcover Rp) {
    pcube last, p, *list;
    sm_matrix *table;
    int size_last_dominance, i;

    /* Mark each cube in DE as not part of the redundant set */
    foreach_set(D, last, p) {
        RESET(p, REDUND);
    }
    foreach_set(E, last, p) {
        RESET(p, REDUND);
    }

    /* Mark each cube in Rp as partially redundant */
    foreach_set(Rp, last, p) {
        SET(p, REDUND); /* belongs to redundant set */
    }

    /* For each cube in Rp, find ways to cover its minterms */
    list = cube3list(D, E, Rp);
    table = sm_alloc();
    size_last_dominance = 0;
    i = 0;
    foreach_set(Rp, last, p) {
        Rp_current = SIZE(p);
        fcube_is_covered(list, p, table);
        RESET(p, REDUND); /* can now consider this cube redundant */

        /* try to keep memory limits down by reducing table as we go along */
        if (table->nrows - size_last_dominance > 1000) {
            (void)sm_row_dominance(table);
            size_last_dominance = table->nrows;
        }
        i++;
    }
    free_cubelist(list);

    return table;
}

/* cube_is_covered -- determine if a cubelist "covers" a single cube */
bool cube_is_covered(pcube *T, pcube c) {
    return tautology(cofactor(T, c));  // Theorem 3.1.2, p33
}

/* tautology -- answer the tautology question for T */
bool tautology(pcube *T /* T will be disposed of */
) {
    pcube cl, cr;
    int best, result;

    if ((result = taut_special_cases(T)) == MAYBE) {
        cl = new_cube();
        cr = new_cube();
        best = binate_split_select(T, cl, cr);
        result = tautology(scofactor(T, cl, best)) &&
                 tautology(scofactor(T, cr, best));  // Proposition 3.1.2, p34
        free_cubelist(T);
        free_cube(cl);
        free_cube(cr);
    }

    return result;
}

/*
 *  taut_special_cases -- check special cases for tautology
 */
bool taut_special_cases(pcube *T /* will be disposed if answer is determined */
) {
    pcube *T1, *Tsave, p, ceil = cube.temp[0], temp = cube.temp[1];
    pcube *A, *B;
    int var;

    /* Check for a row of all 1's which implies tautology */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        if (full_row(p, T[0])) {
            free_cubelist(T);
            return TRUE;
        }
    }

    /* Check for a column of all 0's which implies no tautology */
start:
    INLINEset_copy(ceil, T[0]);
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        INLINEset_or(ceil, ceil, p);
    }
    if (!setp_equal(ceil, cube.fullset)) {
        free_cubelist(T);
        return FALSE;
    }

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If function is unate (and no row of all 1's), then no tautology */
    if (cdata.vars_unate == cdata.vars_active) {
        free_cubelist(T);
        return FALSE;

        /* If active in a single variable (and no column of 0's) then tautology
         */
    } else if (cdata.vars_active == 1) {
        free_cubelist(T);
        return TRUE;

        /* Check for unate variables, and reduce cover if there are any */
    } else if (cdata.vars_unate != 0) {
        /* Form a cube "ceil" with full variables in the unate variables */
        (void)set_copy(ceil, cube.emptyset);
        for (var = 0; var < cube.num_input_vars + 1; var++) {
            if (cdata.is_unate[var]) {
                INLINEset_or(ceil, ceil, cube.var_mask[var]);
            }
        }

        /* Save only those cubes that are "full" in all unate variables */
        for (Tsave = T1 = T + 2; (p = *T1++) != 0;) {
            if (setp_implies(ceil, set_or(temp, p, T[0]))) {
                *Tsave++ = p;
            }
        }
        *Tsave++ = NULL;
        T[1] = (pcube)Tsave;

        goto start;

        /* Check for component reduction */
    } else if (cdata.var_zeros[cdata.best] < CUBELISTSIZE(T) / 2) {
        if (cubelist_partition(T, &A, &B) == 0) {
            return MAYBE;
        } else {
            free_cubelist(T);
            if (tautology(A)) {
                free_cubelist(B);
                return TRUE;
            } else {
                return tautology(B);
            }
        }
    }

    /* We tried as hard as we could, but must recurse from here on */
    return MAYBE;
}
