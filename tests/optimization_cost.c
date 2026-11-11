#include "espresso.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setup(int inputs, int outputs) {
    cube.num_binary_vars = inputs;
    cube.num_vars = inputs + 1;
    cube.part_size = ALLOC(int, cube.num_vars);
    cube.part_size[inputs] = outputs;
    cube_setup();
}

static unsigned evaluate(pcover F, unsigned input) {
    pset p, last;
    unsigned outputs = 0;
    foreach_set(F, last, p) {
        int var;
        for (var = 0; var < cube.num_binary_vars; ++var) {
            unsigned bit = (input >> (cube.num_binary_vars - 1 - var)) & 1;
            if (!is_in_set(p, 2 * var + bit))
                break;
        }
        if (var == cube.num_binary_vars) {
            int output;
            for (output = 0; output < cube.part_size[cube.output]; ++output)
                if (is_in_set(p, cube.first_part[cube.output] + output))
                    outputs |= 1u << output;
        }
    }
    return outputs;
}

static int check(pcover result, pcover original, int products, int literals) {
    unsigned input;
    int input_literals = 0;
    pset p, last;
    foreach_set(result, last, p) {
        int var;
        for (var = 0; var < cube.num_binary_vars; ++var)
            if (!is_in_set(p, 2 * var) || !is_in_set(p, 2 * var + 1))
                ++input_literals;
    }
    for (input = 0; input < (1u << cube.num_binary_vars); ++input) {
        if (evaluate(result, input) != evaluate(original, input)) {
            fprintf(stderr, "truth table differs at input %u\n", input);
            return 1;
        }
    }
    if (result->count > products ||
        (result->count == products && input_literals > literals)) {
        fprintf(stderr,
                "got %d products/%d input literals, expected at most "
                "%d/%d (lexicographic)\n",
                result->count, input_literals, products, literals);
        return 1;
    }
    return 0;
}

static pcover iteration_cover(void) {
    static const unsigned minterms[] = {
        0,  5,  7,  8,  9,  11, 12, 13, 15, 17, 18, 20, 21, 22, 23, 24, 25, 26,
        28, 32, 33, 34, 40, 41, 42, 43, 44, 47, 48, 50, 52, 53, 56, 57, 60, 61};
    pcover F = new_cover(sizeof minterms / sizeof minterms[0]);
    pset p = new_cube();
    size_t i;
    for (i = 0; i < sizeof minterms / sizeof minterms[0]; ++i) {
        int var;
        set_clear(p, cube.size);
        for (var = 0; var < cube.num_binary_vars; ++var)
            set_insert(p, 2 * var + ((minterms[i] >> (5 - var)) & 1));
        set_insert(p, cube.first_part[cube.output]);
        F = sf_addset(F, p);
    }
    set_free(p);
    return F;
}

static pcover sparse_cover(void) {
    static const char *terms[] = {"-001100", "-1-0100", "0--1001", "-0-0001",
                                  "111-110", "1000011", "-011011", "0-11011",
                                  "-100101", "00-0101", "0001111"};
    pcover F = new_cover(sizeof terms / sizeof terms[0]);
    pset p = new_cube();
    size_t i;
    for (i = 0; i < sizeof terms / sizeof terms[0]; ++i) {
        int var;
        set_clear(p, cube.size);
        for (var = 0; var < 4; ++var) {
            if (terms[i][var] != '1')
                set_insert(p, 2 * var);
            if (terms[i][var] != '0')
                set_insert(p, 2 * var + 1);
        }
        for (var = 0; var < 3; ++var)
            if (terms[i][4 + var] == '1')
                set_insert(p, 8 + var);
        SET(p, PRIME);
        F = sf_addset(F, p);
    }
    set_free(p);
    return F;
}

int main(int argc, char **argv) {
    pcover F, D, R, original;
    int sparse, failed;
    if (argc != 2 ||
        (strcmp(argv[1], "iteration") && strcmp(argv[1], "sparse")))
        return 2;
    sparse = !strcmp(argv[1], "sparse");
    setup(sparse ? 4 : 6, sparse ? 3 : 1);
    F = sparse ? sparse_cover() : iteration_cover();
    original = sf_save(F);
    D = new_cover(0);
    R = complement(cube2list(F, D));
    F = sparse ? make_sparse(F, D, R) : espresso(F, D, R);
    /* The single-output example needs another Espresso iteration. The
     * multiple-output example needs another sparse cleanup iteration.
     * Accept better results without depending on the order of the cubes. */
    failed = check(F, original, sparse ? 10 : 12, sparse ? 25 : 48);
    free_cover(F);
    free_cover(D);
    free_cover(R);
    free_cover(original);
    setdown_cube();
    FREE(cube.part_size);
    sf_cleanup();
    sm_cleanup();
    return failed;
}
