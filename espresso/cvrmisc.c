#include "espresso.h"

/* cost -- compute the cost of a cover */
void cover_cost(pcover F, pcost cost) {
    pcube p, last;
    pcube *T;
    int var;

    /* use the routine used by cofactor to decide splitting variables */
    massive_count(T = cube1list(F));
    free_cubelist(T);

    cost->cubes = F->count;
    cost->total = cost->in = cost->out = cost->mv = cost->primes = 0;

    /* Count transistors (zeros) for each binary variable (inputs) */
    for (var = 0; var < cube.num_input_vars; var++)
        cost->in += cdata.var_zeros[var];

    /* Count the transistors (ones) for the output variable */
    cost->out =
        F->count * cube.part_size[cube.output] - cdata.var_zeros[cube.output];

    /* Count the number of nonprime cubes */
    foreach_set(F, last, p) cost->primes += TESTP(p, PRIME) != 0;

    /* Count the total number of literals */
    cost->total = cost->in + cost->out + cost->mv;
}

/* copy_cost -- copy a cost function from s to d */
void copy_cost(pcost s, pcost d) {
    d->cubes = s->cubes;
    d->in = s->in;
    d->out = s->out;
    d->mv = s->mv;
    d->total = s->total;
    d->primes = s->primes;
}

/* fatal -- report fatal error message and take a dive */
void fatal(char *s) {
    fprintf(stderr, "espresso: %s\n", s);
    exit(1);
}
