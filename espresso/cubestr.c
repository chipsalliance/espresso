/*
    Module: cubestr.c -- routines for managing the global cube structure
*/

#include "espresso.h"

/*
    cube_setup -- assume that the fields "num_input_vars", and
    part_size[num_input_vars (aka output)] is setup, and initialize the
    rest of cube and cdata.

    If a part_size is < 0, then the field size is abs(part_size) and the
    field read from the input is symbolic.
*/
void cube_setup() {
    int i, var;
    pcube p;

    cube.output = cube.num_input_vars;
    cube.size = 0;
    cube.first_part = ALLOC(int, cube.num_input_vars + 1);
    cube.last_part = ALLOC(int, cube.num_input_vars + 1);
    cube.first_word = ALLOC(int, cube.num_input_vars + 1);
    cube.last_word = ALLOC(int, cube.num_input_vars + 1);
    for (var = 0; var < cube.num_input_vars + 1; var++) {
        if (var < cube.num_input_vars)
            cube.part_size[var] = 2;
        cube.first_part[var] = cube.size;
        cube.first_word[var] = WHICH_WORD(cube.size);
        cube.size += cube.part_size[var];
        cube.last_part[var] = cube.size - 1;
        cube.last_word[var] = WHICH_WORD(cube.size - 1);
    }

    cube.var_mask = ALLOC(pset, cube.num_input_vars + 1);
    cube.sparse = ALLOC(int, cube.num_input_vars + 1);
    cube.binary_mask = new_cube();
    for (var = 0; var < cube.num_input_vars + 1; var++) {
        p = cube.var_mask[var] = new_cube();
        for (i = cube.first_part[var]; i <= cube.last_part[var]; i++)
            set_insert(p, i);
        if (var < cube.num_input_vars) {
            INLINEset_or(cube.binary_mask, cube.binary_mask, p);
            cube.sparse[var] = 0;
        } else {  // output
            cube.sparse[var] = 1;
        }
    }
    if (cube.num_input_vars == 0)
        cube.inword = -1;
    else {
        cube.inword = cube.last_word[cube.num_input_vars - 1];
        cube.inmask = cube.binary_mask[cube.inword] & DISJOINT;
    }

    cube.temp = ALLOC(pset, CUBE_TEMP);
    for (i = 0; i < CUBE_TEMP; i++)
        cube.temp[i] = new_cube();
    cube.fullset = set_fill(new_cube(), cube.size);
    cube.emptyset = new_cube();

    cdata.part_zeros = ALLOC(int, cube.size);
    cdata.var_zeros = ALLOC(int, cube.num_input_vars + 1);
    cdata.parts_active = ALLOC(int, cube.num_input_vars + 1);
    cdata.is_unate = ALLOC(int, cube.num_input_vars + 1);
}

/*
    setdown_cube -- free memory allocated for the cube/cdata structs
    (free's all but the part_size array)

    (I wanted to call this cube_setdown, but that violates the 8-character
    external routine limit on the IBM !)
*/
void setdown_cube() {
    int i, var;

    FREE(cube.first_part);
    FREE(cube.last_part);
    FREE(cube.first_word);
    FREE(cube.last_word);
    FREE(cube.sparse);

    free_cube(cube.binary_mask);
    free_cube(cube.fullset);
    free_cube(cube.emptyset);
    for (var = 0; var < cube.num_input_vars + 1; var++)
        free_cube(cube.var_mask[var]);
    FREE(cube.var_mask);

    for (i = 0; i < CUBE_TEMP; i++)
        free_cube(cube.temp[i]);
    FREE(cube.temp);

    FREE(cdata.part_zeros);
    FREE(cdata.var_zeros);
    FREE(cdata.parts_active);
    FREE(cdata.is_unate);

    cube.first_part = cube.last_part = (int *)NULL;
    cube.first_word = cube.last_word = (int *)NULL;
    cube.sparse = (int *)NULL;
    cube.binary_mask = (pcube)NULL;
    cube.fullset = cube.emptyset = (pcube)NULL;
    cube.var_mask = cube.temp = (pcube *)NULL;

    cdata.part_zeros = cdata.var_zeros = cdata.parts_active = (int *)NULL;
    cdata.is_unate = (bool *)NULL;
}
