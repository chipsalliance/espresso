/*
    module: cvrout.c
    purpose: cube and cover output routines
*/

#include "espresso.h"

void fprint_pla(FILE *fp, pPLA PLA) {
    pcube last, p;

    fprintf(fp, ".i %d\n", cube.num_binary_vars);
    fprintf(fp, ".o %d\n", cube.part_size[cube.output]);

    fprintf(fp, ".type f\n");

    foreach_set(PLA->F, last, p) {
        print_cube(fp, p, "01");
    }
    fprintf(fp, ".e\n");
}

void print_cube(FILE *fp, pcube c, char *out_map) {
    int i, var, ch;
    int last;

    for (var = 0; var < cube.num_binary_vars; var++) {
        ch = "?01-"[GETINPUT(c, var)];
        putc(ch, fp);
    }
    for (var = cube.num_binary_vars; var < cube.num_vars - 1; var++) {
        putc(' ', fp);
        for (i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
            ch = "01"[is_in_set(c, i) != 0];
            putc(ch, fp);
        }
    }
    if (cube.output != -1) {
        last = cube.last_part[cube.output];
        putc(' ', fp);
        for (i = cube.first_part[cube.output]; i <= last; i++) {
            ch = out_map[is_in_set(c, i) != 0];
            putc(ch, fp);
        }
    }
    putc('\n', fp);
}
