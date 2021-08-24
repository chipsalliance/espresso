/*
    contain.c -- set containment routines

    These are complex routines for performing containment over a
    family of sets, but they have the advantage of being much faster
    than a straightforward n*n routine.

    First the cubes are sorted by size, and as a secondary key they are
    sorted so that if two cubes are equal they end up adjacent.  We can
    than quickly remove equal cubes from further consideration by
    comparing each cube to its neighbor.  Finally, because the cubes
    are sorted by size, we need only check cubes which are larger (or
    smaller) than a given cube for containment.
*/

#include "espresso.h"

/*
    sf_contain -- perform containment on a set family (delete sets which
    are contained by some larger set in the family).  No assumptions are
    made about A, and the result will be returned in decreasing order of
    set size.
*/
pset_family sf_contain(pset_family A /* disposes of A */
) {
    int cnt;
    pset *A1;
    pset_family R;

    A1 = sf_sort(A, descend);           /* sort into descending order */
    rm_equal(A1, descend);              /* remove duplicates */
    cnt = rm_contain(A1);               /* remove contained sets */
    R = sf_unlist(A1, cnt, A->sf_size); /* recreate the set family */
    sf_free(A);
    return R;
}

/*
    sf_rev_contain -- perform containment on a set family (delete sets which
    contain some smaller set in the family).  No assumptions are made about
    A, and the result will be returned in increasing order of set size
*/
pset_family sf_rev_contain(pset_family A /* disposes of A */
) {
    int cnt;
    pset *A1;
    pset_family R;

    A1 = sf_sort(A, ascend);            /* sort into ascending order */
    rm_equal(A1, ascend);               /* remove duplicates */
    cnt = rm_rev_contain(A1);           /* remove containing sets */
    R = sf_unlist(A1, cnt, A->sf_size); /* recreate the set family */
    sf_free(A);
    return R;
}

/* sf_dupl -- delete duplicate sets in a set family */
pset_family sf_dupl(pset_family A /* disposes of A */
) {
    int cnt;
    pset *A1;
    pset_family R;

    A1 = sf_sort(A, descend);           /* sort the set family */
    cnt = rm_equal(A1, descend);        /* remove duplicates */
    R = sf_unlist(A1, cnt, A->sf_size); /* recreate the set family */
    sf_free(A);
    return R;
}

/* rm_equal -- scan a sorted array of set pointers for duplicate sets */
int rm_equal(pset *A1, /* updated in place */
             int (*compare)(pset *, pset *)) {
    pset *p, *pdest = A1;

    if (*A1 != NULL) { /* If more than one set */
        for (p = A1 + 1; *p != NULL; p++)
            if ((*compare)(p, p - 1) != 0)
                *pdest++ = *(p - 1);
        *pdest++ = *(p - 1);
        *pdest = NULL;
    }
    return pdest - A1;
}

/* rm_contain -- perform containment over a sorted array of set pointers */
int rm_contain(pset *A1 /* updated in place */
) {
    pset *pa, *pb, *pcheck, a, b;
    pset *pdest = A1;
    int last_size = -1;

    /* Loop for all cubes of A1 */
    for (pa = A1; (a = *pa++) != NULL;) {
        /* Update the check pointer if the size has changed */
        if (SIZE(a) != last_size)
            last_size = SIZE(a), pcheck = pdest;
        for (pb = A1; pb != pcheck;) {
            b = *pb++;
            INLINEsetp_implies(a, b, /* when_false => */ continue);
            goto lnext1;
        }
        /* set a was not contained by some larger set, so save it */
        *pdest++ = a;
    lnext1:;
    }

    *pdest = NULL;
    return pdest - A1;
}

/* rm_rev_contain -- perform rcontainment over a sorted array of set pointers */
int rm_rev_contain(pset *A1 /* updated in place */
) {
    pset *pa, *pb, *pcheck, a, b;
    pset *pdest = A1;
    int last_size = -1;

    /* Loop for all cubes of A1 */
    for (pa = A1; (a = *pa++) != NULL;) {
        /* Update the check pointer if the size has changed */
        if (SIZE(a) != last_size)
            last_size = SIZE(a), pcheck = pdest;
        for (pb = A1; pb != pcheck;) {
            b = *pb++;
            INLINEsetp_implies(b, a, /* when_false => */ continue);
            goto lnext1;
        }
        /* the set a did not contain some smaller set, so save it */
        *pdest++ = a;
    lnext1:;
    }

    *pdest = NULL;
    return pdest - A1;
}

/* sf_sort -- sort the sets of A */
pset *sf_sort(pset_family A, int (*compare)(pset *, pset *)) {
    pset p, last, *pdest, *A1;

    /* Create a single array pointing to each cube of A */
    pdest = A1 = ALLOC(pset, A->count + 1);
    foreach_set(A, last, p) {
        PUTSIZE(p, set_ord(p)); /* compute the set size */
        *pdest++ = p;           /* save the pointer */
    }
    *pdest = NULL; /* Sentinel -- never seen by sort */

    /* Sort cubes by size */
    qsort((char *)A1, A->count, sizeof(pset),
          (int (*)(const void *, const void *))compare);
    return A1;
}

/* sf_list -- make a list of pointers to the sets in a set family */
pset *sf_list(pset_family A) {
    pset p, last, *pdest, *A1;

    /* Create a single array pointing to each cube of A */
    pdest = A1 = ALLOC(pset, A->count + 1);
    foreach_set(A, last, p) *pdest++ = p; /* save the pointer */
    *pdest = NULL;                        /* Sentinel */
    return A1;
}

/* sf_unlist -- make a set family out of a list of pointers to sets */
pset_family sf_unlist(pset *A1, int totcnt, int size) {
    pset pr, p, *pa;
    pset_family R = sf_new(totcnt, size);

    R->count = totcnt;
    for (pr = R->data, pa = A1; (p = *pa++) != NULL; pr += R->wsize)
        INLINEset_copy(pr, p);
    FREE(A1);
    return R;
}

/* d1_rm_equal -- distance-1 merge (merge cubes which are equal under a mask) */
int d1_rm_equal(pset *A1,                      /* array of set pointers */
                int (*compare)(pset *, pset *) /* comparison function */
) {
    int i, j, dest;

    dest = 0;
    if (A1[0] != (pcube)NULL) {
        for (i = 0, j = 1; A1[j] != (pcube)NULL; j++)
            if ((*compare)(&A1[i], &A1[j]) == 0) {
                /* if sets are equal (under the mask) merge them */
                set_or(A1[i], A1[i], A1[j]);
            } else {
                /* sets are unequal, so save the set i */
                A1[dest++] = A1[i];
                i = j;
            }
        A1[dest++] = A1[i];
    }
    A1[dest] = (pcube)NULL;
    return dest;
}

/*
    dist_merge -- consider all sets to be "or"-ed with "mask" and then
    delete duplicates from the set family.
*/
pset_family dist_merge(pset_family A, /* disposes of A */
                       pset mask      /* defines variables to mask out */
) {
    pset *A1;
    int cnt;
    pset_family R;

    set_copy(cube.temp[0], mask);
    A1 = sf_sort(A, d1_order);
    cnt = d1_rm_equal(A1, d1_order);
    R = sf_unlist(A1, cnt, A->sf_size);
    sf_free(A);
    return R;
}

/*
    d1merge -- perform an efficient distance-1 merge of cubes of A
*/
pset_family d1merge(pset_family A, /* disposes of A */
                    int var) {
    return dist_merge(A, cube.var_mask[var]);
}
