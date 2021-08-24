#include "espresso.h"

int main() {
    pPLA PLA;

    /* the remaining arguments are argv[optind ... argc-1] */
    PLA = NIL(PLA_t);
    if (read_pla(stdin, &PLA) == EOF) {
        fprintf(stderr, "Unable to find PLA on stdin\n");
        exit(1);
    }

    /*
     *  Now run espresso
     */
    PLA->F = espresso(PLA->F, PLA->D, PLA->R);

    /* Output the solution */
    fprint_pla(stdout, PLA);

    /* cleanup all used memory */
    free_PLA(PLA);
    FREE(cube.part_size);
    setdown_cube(); /* free the cube/cdata structure data */
    sf_cleanup();   /* free unused set structures */
    sm_cleanup();   /* sparse matrix cleanup */

    exit(0);
}
