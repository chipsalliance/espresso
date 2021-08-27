/*
 *  Module: espresso.c
 *  Purpose: The main espresso algorithm
 *
 *  Returns a minimized version of the ON-set of a function
 *
 *  The following global variables affect the operation of Espresso:
 *
 *  MISCELLANEOUS:
 *      trace
 *          print trace information as the minimization progresses
 *
 *      remove_essential
 *          remove essential primes
 *
 *      single_expand
 *          if true, stop after first expand/irredundant
 *
 *  LAST_GASP or SUPER_GASP strategy:
 *      use_super_gasp
 *          uses the super_gasp strategy rather than last_gasp
 *
 *  SETUP strategy:
 *      recompute_onset
 *          recompute onset using the complement before starting
 *
 *      unwrap_onset
 *          unwrap the function output part before first expand
 *
 *  MAKE_SPARSE strategy:
 *      force_irredundant
 *          iterates make_sparse to force a minimal solution (used
 *          indirectly by make_sparse)
 *
 *      skip_make_sparse
 *          skip the make_sparse step (used by opo only)
 */

#include "espresso.h"

pcover espresso(pcover F, pcover D1, pcover R) {
    pcover E, D, Fsave;
    pset last, p;
    cost_t cost, best_cost;
    bool unwrap_onset = TRUE;

begin:
    Fsave = sf_save(F); /* save original function */
    D = sf_save(D1);    /* make a scratch copy of D */

    /* Setup has always been a problem */
    cover_cost(F, &cost);
    if (unwrap_onset && (cube.part_size[cube.num_vars - 1] > 1) &&
        (cost.out != cost.cubes * cube.part_size[cube.num_vars - 1]) &&
        (cost.out < 5000))
        F = sf_contain(unravel_output(F));

    /* Initial expand and irredundant */
    foreach_set(F, last, p) {
        RESET(p, PRIME);
    }
    F = expand(F, R, FALSE);
    F = irredundant(F, D);

    E = essential(&F, &D);

    cover_cost(F, &cost);
    do {
        /* Repeat inner loop until solution becomes "stable" */
        do {
            copy_cost(&cost, &best_cost);
            F = reduce(F, D);
            F = expand(F, R, FALSE);
            F = irredundant(F, D);
        } while (cost.cubes < best_cost.cubes);

        /* Perturb solution to see if we can continue to iterate */
        copy_cost(&cost, &best_cost);

        F = last_gasp(F, D, R);

    } while (cost.cubes < best_cost.cubes ||
             (cost.cubes == best_cost.cubes && cost.total < best_cost.total));

    /* Append the essential cubes to F */
    F = sf_append(F, E); /* disposes of E */

    /* Free the D which we used */
    free_cover(D);

    /* Attempt to make the PLA matrix sparse */
    F = make_sparse(F, D1, R);

    /*
     *  Check to make sure function is actually smaller !!
     *  This can only happen because of the initial unravel.  If we fail,
     *  then run the whole thing again without the unravel.
     */
    if (Fsave->count < F->count) {
        free_cover(F);
        F = Fsave;
        unwrap_onset = FALSE;
        goto begin;
    } else {
        free_cover(Fsave);
    }

    return F;
}
