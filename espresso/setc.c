/*
    setc.c -- massive bit-hacking for performing special "cube"-type
    operations on a set

    The basic trick used for binary valued variables is the following:

    If a[w] and b[w] contain a full word of binary variables, then:

     1) to get the full word of their intersection, we use

            x = a[w] & b[w];


     2) to see if the intersection is null in any variables, we examine

            x = ~(x | x >> 1) & DISJOINT;

        this will have a single 1 in each binary variable for which
        the intersection is null.  In particular, if this is zero,
        then there are no disjoint variables; or, if this is nonzero,
        then there is at least one disjoint variable.  A "count_ones"
        over x will tell in how many variables they have an null
        intersection.


     3) to get a mask which selects the disjoint variables, we use

            (x | x << 1)

        this provides a selector which can be used to see where
        they have an null intersection


    cdist       return distance between two cubes
    consensus   compute consensus of two cubes distance 1 apart
    force_lower expand hack (for now), related to consensus
*/

#include "espresso.h"

/* see if the cube has a full row of 1's (with respect to cof) */
bool full_row(pcube p, pcube cof) {
    int i = LOOP(p);
    do
        if ((p[i] | cof[i]) != cube.fullset[i])
            return FALSE;
    while (--i > 0);
    return TRUE;
}

/*
    cdist -- return the "distance" between two cubes (defined as the
    number of null variables in their intersection).

    a   b   contribution
    0   0        0
    0   1        1
    0   -        0
    1   0        1
    1   1        0
    1   -        0
    -   0        0
    -   1        0
    -   -        0
*/

int cdist(pset a, pset b) {
    int dist = 0;

    { /* Check binary variables */
        int w, last = cube.inword;
        unsigned int x;
        /* Check the partial word of binary variables */
        x = a[last] & b[last];
        if ((x = ~(x | x >> 1) & cube.inmask))
            dist = count_ones(x);

        /* Check the full words of binary variables */
        for (w = 1; w < last; w++) {
            x = a[w] & b[w];
            if ((x = ~(x | x >> 1) & DISJOINT))
                dist += count_ones(x);
        }
    }

    { /* Check the output variable */
        int w;
        for (w = cube.first_word[cube.output]; w <= cube.last_word[cube.output];
             w++)
            if (a[w] & b[w] & cube.var_mask[cube.output][w])
                return dist;
        dist++;
    }
    return dist;
}

/*
    force_lower -- Determine which variables of a do not intersect b.
*/

pset force_lower(pset xlower, pset a, pset b) {
    { /* Check binary variables (if any) */
        int w, last = cube.inword;
        unsigned int x;
        /* Check the partial word of binary variables */
        x = a[last] & b[last];
        if ((x = ~(x | x >> 1) & cube.inmask))
            xlower[last] |= (x | (x << 1)) & a[last];

        /* Check the full words of binary variables */
        for (w = 1; w < last; w++) {
            x = a[w] & b[w];
            if ((x = ~(x | x >> 1) & DISJOINT))
                xlower[w] |= (x | (x << 1)) & a[w];
        }
    }

    { /* Check the output variable */
        int w;
        for (w = cube.first_word[cube.output]; w <= cube.last_word[cube.output];
             w++)
            if (a[w] & b[w] & cube.var_mask[cube.output][w])
                return xlower;
        for (w = cube.first_word[cube.output]; w <= cube.last_word[cube.output];
             w++)
            xlower[w] |= a[w] & cube.var_mask[cube.output][w];
    }
    return xlower;
}

/*
    consensus -- multiple-valued consensus

    Although this looks very messy, the idea is to compute for r the
    "and" of the cubes a and b for each variable, unless the "and" is
    null in a variable, in which case the "or" of a and b is computed
    for this variable.

    Because we don't check how many variables are null in the
    intersection of a and b, the returned value for r really only
    represents the consensus when a and b are distance 1 apart.
*/

void consensus(pcube r, pcube a, pcube b) {
    INLINEset_clear(r, cube.size);

    { /* Check binary variables (if any) */
        int w, last = cube.inword;
        unsigned int x;
        /* Check the partial word of binary variables */
        r[last] = x = a[last] & b[last];
        if ((x = ~(x | x >> 1) & cube.inmask))
            r[last] |= (x | (x << 1)) & (a[last] | b[last]);

        /* Check the full words of binary variables */
        for (w = 1; w < last; w++) {
            r[w] = x = a[w] & b[w];
            if ((x = ~(x | x >> 1) & DISJOINT))
                r[w] |= (x | (x << 1)) & (a[w] | b[w]);
        }
    }

    { /* Check the output variable */
        bool empty;
        unsigned int x;
        int w;
        empty = TRUE;
        for (w = cube.first_word[cube.output]; w <= cube.last_word[cube.output];
             w++)
            if ((x = a[w] & b[w] & cube.var_mask[cube.output][w]))
                empty = FALSE, r[w] |= x;
        if (empty)
            for (w = cube.first_word[cube.output];
                 w <= cube.last_word[cube.output]; w++)
                r[w] |= cube.var_mask[cube.output][w] & (a[w] | b[w]);
    }
}

/*
    cactive -- return the index of the single active variable in
    the cube, or return -1 if there are none or more than 2.
*/

int cactive(pcube a) {
    int active = -1, dist = 0;

    { /* Check binary variables */
        int w, last = cube.inword;
        unsigned int x;
        /* Check the partial word of binary variables */
        x = a[last];
        if ((x = ~(x & x >> 1) & cube.inmask)) {
            if ((dist = count_ones(x)) > 1)
                return -1; /* more than 2 active variables */
            active = (last - 1) * (BPI / 2) + bit_index(x) / 2;
        }

        /* Check the full words of binary variables */
        for (w = 1; w < last; w++) {
            x = a[w];
            if ((x = ~(x & x >> 1) & DISJOINT)) {
                if ((dist += count_ones(x)) > 1)
                    return -1; /* more than 2 active variables */
                active = (w - 1) * (BPI / 2) + bit_index(x) / 2;
            }
        }
    }

    { /* Check the output variable */
        int w;
        for (w = cube.first_word[cube.output]; w <= cube.last_word[cube.output];
             w++)
            if (cube.var_mask[cube.output][w] & ~a[w]) {
                if (++dist > 1)
                    return -1;
                active = cube.output;
                break;
            }
    }
    return active;
}

/*
    ccommon -- return TRUE if a and b are share "active" variables
    active variables include variables that are empty;
*/

bool ccommon(pcube a, pcube b, pcube cof) {
    { /* Check binary variables */
        int last = cube.inword;
        int w;
        unsigned int x, y;
        /* Check the partial word of binary variables */
        x = a[last] | cof[last];
        y = b[last] | cof[last];
        if (~(x & x >> 1) & ~(y & y >> 1) & cube.inmask)
            return TRUE;

        /* Check the full words of binary variables */
        for (w = 1; w < last; w++) {
            x = a[w] | cof[w];
            y = b[w] | cof[w];
            if (~(x & x >> 1) & ~(y & y >> 1) & DISJOINT)
                return TRUE;
        }
    }

    { /* Check the output variable */
        int w;
        /* Check for some part missing from a */
        for (w = cube.first_word[cube.output]; w <= cube.last_word[cube.output];
             w++)
            if (cube.var_mask[cube.output][w] & ~a[w] & ~cof[w]) {
                /* If so, check for some part missing from b */
                for (w = cube.first_word[cube.output];
                     w <= cube.last_word[cube.output]; w++)
                    if (cube.var_mask[cube.output][w] & ~b[w] & ~cof[w])
                        return TRUE; /* both active */
                break;
            }
    }
    return FALSE;
}

/*
    These routines compare two sets (cubes) for the qsort() routine and
    return:

        -1 if set a is to precede set b
         0 if set a and set b are equal
         1 if set a is to follow set b

    Usually the SIZE field of the set is assumed to contain the size
    of the set (which will save recomputing the set size during the
    sort).  For distance-1 merging, the global variable cube.temp[0] is
    a mask which mask's-out the merging variable.
*/

/* descend -- comparison for descending sort on set size */
int descend(pset *a, pset *b) {
    pset a1 = *a, b1 = *b;
    if (SIZE(a1) > SIZE(b1))
        return -1;
    else if (SIZE(a1) < SIZE(b1))
        return 1;
    else {
        int i = LOOP(a1);
        do
            if (a1[i] > b1[i])
                return -1;
            else if (a1[i] < b1[i])
                return 1;
        while (--i > 0);
    }
    return 0;
}

/* ascend -- comparison for ascending sort on set size */
int ascend(pset *a, pset *b) {
    pset a1 = *a, b1 = *b;
    if (SIZE(a1) > SIZE(b1))
        return 1;
    else if (SIZE(a1) < SIZE(b1))
        return -1;
    else {
        int i = LOOP(a1);
        do
            if (a1[i] > b1[i])
                return 1;
            else if (a1[i] < b1[i])
                return -1;
        while (--i > 0);
    }
    return 0;
}

/* d1_order -- comparison for distance-1 merge routine */
int d1_order(pset *a, pset *b) {
    pset a1 = *a, b1 = *b, c1 = cube.temp[0];
    int i = LOOP(a1);
    unsigned int x1, x2;
    do
        if ((x1 = a1[i] | c1[i]) > (x2 = b1[i] | c1[i]))
            return -1;
        else if (x1 < x2)
            return 1;
    while (--i > 0);
    return 0;
}
