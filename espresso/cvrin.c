/*
    module: cvrin.c
    purpose: cube and cover input routines
*/

#include "espresso.h"

static bool line_length_error;
static int lineno;

pla_type_t pla_type = TYPE_FD;

void skip_line(FILE *fpin) {
    int ch;
    while ((ch = getc(fpin)) != EOF && ch != '\n')
        ;
    lineno++;
}

char *get_word(FILE *fp, char *word) {
    int ch, i = 0;
    while ((ch = getc(fp)) != EOF && isspace(ch))
        ;
    word[i++] = ch;
    while ((ch = getc(fp)) != EOF && !isspace(ch))
        word[i++] = ch;
    word[i] = '\0';
    return word;
}

/*
 *  Yes, I know this routine is a mess
 */
void read_cube(FILE *fp, pPLA PLA) {
    int var, i;
    pcube cf = cube.temp[0], cr = cube.temp[1], cd = cube.temp[2];
    bool savef = FALSE, saved = FALSE, saver = FALSE;

    set_clear(cf, cube.size);

    /* Loop and read binary variables */
    for (var = 0; var < cube.num_input_vars; var++)
        switch (getc(fp)) {
            case EOF:
                goto bad_char;
            case '\n':
                if (!line_length_error)
                    fprintf(stderr, "product term(s) %s\n",
                            "span more than one line (warning only)");
                line_length_error = TRUE;
                lineno++;
                var--;
                break;
            case ' ':
            case '|':
            case '\t':
                var--;
                break;
            case '2':
            case '-':  // 2n and (2n + 1)
                set_insert(cf, var * 2 + 1);
            case '0':  // 2n
                set_insert(cf, var * 2);
                break;
            case '1':  // (2n + 1)
                set_insert(cf, var * 2 + 1);
                break;
            case '?':
                break;
            default:
                goto bad_char;
        }

    /* Loop for last multiple-valued variable (the output) */
    set_copy(cr, cf);
    set_copy(cd, cf);
    for (i = cube.first_part[var]; i <= cube.last_part[var]; i++)
        switch (getc(fp)) {
            case EOF:
                goto bad_char;
            case '\n':
                if (!line_length_error)
                    fprintf(stderr, "product term(s) %s\n",
                            "span more than one line (warning only)");
                line_length_error = TRUE;
                lineno++;
                i--;
                break;
            case ' ':
            case '|':
            case '\t':
                i--;
                break;
            case '4':
            case '1':
                set_insert(cf, i), savef = TRUE;
                break;
            case '3':
            case '0':
                if (pla_type == TYPE_FR)
                    set_insert(cr, i), saver = TRUE;
                break;
            case '2':
            case '-':
                if (pla_type == TYPE_FD)
                    set_insert(cd, i), saved = TRUE;
            case '~':
                break;
            default:
                goto bad_char;
        }
    if (savef)
        PLA->F = sf_addset(PLA->F, cf);
    if (saved)
        PLA->D = sf_addset(PLA->D, cd);
    if (saver)
        PLA->R = sf_addset(PLA->R, cr);
    return;

bad_char:
    fprintf(stderr, "(warning): input line #%d ignored\n", lineno);
    skip_line(fp);
    return;
}

void parse_pla(FILE *fp, pPLA PLA) {
    int ch;
    char word[256];

    lineno = 1;
    line_length_error = FALSE;

    while (1) {
        switch (ch = getc(fp)) {
            case EOF:
                return;

            case '\n':
                lineno++;

            case ' ':
            case '\t':
            case '\f':
            case '\r':
                break;

            case '.':
                /* .i gives the cube input size (binary-functions only) */
                if (equal(get_word(fp, word), "i")) {
                    if (cube.fullset != NULL) {
                        fprintf(stderr, "extra .i ignored\n");
                        skip_line(fp);
                    } else {
                        if (fscanf(fp, "%d", &cube.num_input_vars) != 1)
                            fatal("error reading .i");
                        if (cube.num_input_vars <= 0)
                            fatal("silly value in .i");
                        cube.part_size = ALLOC(int, cube.num_input_vars + 1);
                    }

                    /* .o gives the cube output size (binary-functions only) */
                } else if (equal(word, "o")) {
                    if (cube.fullset != NULL) {
                        fprintf(stderr, "extra .o ignored\n");
                        skip_line(fp);
                    } else {
                        if (cube.part_size == NULL)
                            fatal(".o cannot appear before .i");
                        if (fscanf(fp, "%d",
                                   &(cube.part_size[cube.num_input_vars])) !=
                            1)  // we don't have cube.output until cube_setup is
                                // called
                            fatal("error reading .o");
                        if (cube.part_size[cube.num_input_vars] <= 0)
                            fatal("silly value in .i");
                        cube_setup();
                    }

                    /* .type specifies a logical type for the PLA */
                } else if (equal(word, "type")) {
                    (void)get_word(fp, word);
                    if (equal(word, "fd")) {
                        pla_type = TYPE_FD;
                    } else if (equal(word, "fr")) {
                        pla_type = TYPE_FR;
                    } else {
                        fatal("unknown type in .type");
                    }

                    /* .e and .end specify the end of the file */
                } else if (equal(word, "e") || equal(word, "end"))
                    return;
                /* .p is ignored */
                else if (equal(word, "p"))
                    skip_line(fp);
                else {
                    fprintf(stderr, "%c%s unrecognized\n", ch, word);
                    skip_line(fp);
                }
                break;
            default:
                (void)ungetc(ch, fp);
                if (cube.fullset == NULL) {
                    /*		fatal("unknown PLA size, need .i/.o or .mv");*/
                    skip_line(fp);
                    break;
                }
                if (PLA->F == NULL) {
                    PLA->F = new_cover(10);
                    PLA->D = new_cover(10);
                    PLA->R = new_cover(10);
                }
                read_cube(fp, PLA);
        }
    }
}

/*
    read_pla -- read a PLA from a file

    Input stops when ".e" is encountered in the input file, or upon reaching
    end of file.

    Returns the PLA in the variable PLA after massaging the "symbolic"
    representation into a positional cube notation of the ON-set, OFF-set,
    and the DC-set.

    Returns a status code as a result:
        EOF (-1) : End of file reached before any data was read
        > 0	 : Operation successful
*/
int read_pla(FILE *fp, pPLA *PLA_return) {
    pPLA PLA;

    /* Allocate and initialize the PLA structure */
    PLA = *PLA_return = new_PLA();

    /* Read the pla */
    parse_pla(fp, PLA);

    /* Check for nothing on the file -- implies reached EOF */
    if (PLA->F == NULL) {
        return EOF;
    }

    if (pla_type == TYPE_FD) {
        free_cover(PLA->R);
        PLA->R = complement(cube2list(PLA->F, PLA->D));  // R = U - (F u D)
    } else if (pla_type == TYPE_FR) {
        pcover X;
        free_cover(PLA->D);
        /* hack, why not? */
        X = d1merge(sf_join(PLA->F, PLA->R), cube.num_input_vars);
        PLA->D = complement(cube1list(X));
        free_cover(X);
    }

    return 1;
}

pPLA new_PLA() {
    pPLA PLA;

    PLA = ALLOC(PLA_t, 1);
    PLA->F = PLA->D = PLA->R = (pcover)NULL;
    return PLA;
}

void free_PLA(pPLA PLA) {
    if (PLA->F != (pcover)NULL)
        free_cover(PLA->F);
    if (PLA->R != (pcover)NULL)
        free_cover(PLA->R);
    if (PLA->D != (pcover)NULL)
        FREE(PLA);
}
