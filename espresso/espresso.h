/*
 *  espresso.h -- header file for Espresso-mv
 */

#include "port.h"
#include "sparse.h"
#include "mincov.h"

/*-----THIS USED TO BE set.h----- */

/*
 *  set.h -- definitions for packed arrays of bits
 *
 *   This header file describes the data structures which comprise a
 *   facility for efficiently implementing packed arrays of bits
 *   (otherwise known as sets, cf. Pascal).
 *
 *   A set is a vector of bits and is implemented here as an array of
 *   unsigned integers.  The low order bits of set[0] give the index of
 *   the last word of set data.  The higher order bits of set[0] are
 *   used to store data associated with the set.  The set data is
 *   contained in elements set[1] ... set[LOOP(set)] as a packed bit
 *   array.
 *
 *   A family of sets is a two-dimensional matrix of bits and is
 *   implemented with the data type "set_family".
 *
 *   BPI == 32 and BPI == 16 have been tested and work.
 */

/* Define host machine characteristics of "unsigned int" */
#define BPI 32 /* # bits per integer */

#define LOGBPI 5 /* log(BPI)/log(2) */

/* Define the set type */
typedef unsigned int *pset;

/* Define the set family type -- an array of sets */
typedef struct set_family {
    int wsize;               /* Size of each set in 'ints' */
    int sf_size;             /* User declared set size */
    int capacity;            /* Number of sets allocated */
    int count;               /* The number of sets in the family */
    int active_count;        /* Number of "active" sets */
    pset data;               /* Pointer to the set data */
    struct set_family *next; /* For garbage collection */
} set_family_t, *pset_family;

/* Macros to set and test single elements */
#define WHICH_WORD(element) (((element) >> LOGBPI) + 1)
#define WHICH_BIT(element)  ((element) & (BPI - 1))

/* # of ints needed to allocate a set with "size" elements */
#define SET_SIZE(size) ((size) <= BPI ? 2 : (WHICH_WORD((size)-1) + 1))

/*
 *  Three fields are maintained in the first word of the set
 *      LOOP is the index of the last word used for set data
 *      LOOPCOPY is the index of the last word in the set
 *      SIZE is available for general use (e.g., recording # elements in set)
 *      NELEM retrieves the number of elements in the set
 */
#define LOOP(set)          ((set)[0] & 0x03ff)
#define PUTLOOP(set, i)    ((set)[0] &= ~0x03ff, (set)[0] |= (i))
#define LOOPCOPY(set)      LOOP(set)
#define SIZE(set)          ((set)[0] >> 16)
#define PUTSIZE(set, size) ((set)[0] &= 0xffff, (set)[0] |= ((size) << 16))

#define NELEM(set)     (BPI * LOOP(set))
#define LOOPINIT(size) (((size) <= BPI) ? 1 : WHICH_WORD((size)-1))

/*
 *      FLAGS store general information about the set
 */
#define SET(set, flag)   ((set)[0] |= (flag))
#define RESET(set, flag) ((set)[0] &= ~(flag))
#define TESTP(set, flag) ((set)[0] & (flag))

/* Flag definitions are ... */
#define PRIME    0x8000 /* cube is prime */
#define NONESSEN 0x4000 /* cube cannot be essential prime */
#define ACTIVE   0x2000 /* cube is still active */
#define REDUND   0x1000 /* cube is redundant(at this point) */
#define COVERED  0x0800 /* cube has been covered */
#define RELESSEN 0x0400 /* cube is relatively essential */

/* Most efficient way to look at all members of a set family */
#define foreach_set(R, last, p)                                   \
    for ((p) = (R)->data, (last) = (p) + (R)->count * (R)->wsize; \
         (p) < (last); (p) += (R)->wsize)
#define foreach_remaining_set(R, last, pfirst, p)     \
    for ((p) = (pfirst) + (R)->wsize,                 \
        (last) = (R)->data + (R)->count * (R)->wsize; \
         (p) < (last); (p) += (R)->wsize)
#define foreach_active_set(R, last, p) \
    foreach_set(R, last, p) if (TESTP(p, ACTIVE))

/* Another way that also keeps the index of the current set member in i */
#define foreachi_set(R, i, p) \
    for ((p) = (R)->data, (i) = 0; (i) < (R)->count; (p) += (R)->wsize, (i)++)

/* Looping over all elements in a set:
 *      foreach_set_element(pset p, int i, unsigned val, int base) {
 *		.
 *		.
 *		.
 *      }
 */
#define foreach_set_element(p, i, val, base)                       \
    for ((i) = LOOP(p); (i) > 0;)                                  \
        for ((val) = (p)[i], (base) = --(i) << LOGBPI; (val) != 0; \
             (base)++, (val) >>= 1)                                \
            if ((val)&1)

/* Return a pointer to a given member of a set family */
#define GETSET(family, index) ((family)->data + (family)->wsize * (index))

/* Allocate and deallocate sets */
#define set_new(size) set_clear(ALLOC(unsigned int, SET_SIZE(size)), size)
#define set_save(r)   set_copy(ALLOC(unsigned int, SET_SIZE(NELEM(r))), r)
#define set_free(r)   FREE(r)

/* Check for set membership, remove set element and insert set element */
#define is_in_set(set, e)  ((set)[WHICH_WORD(e)] & (1 << WHICH_BIT(e)))
#define set_remove(set, e) ((set)[WHICH_WORD(e)] &= ~(1 << WHICH_BIT(e)))
#define set_insert(set, e) ((set)[WHICH_WORD(e)] |= 1 << WHICH_BIT(e))

#define INLINEset_copy(r, a)   \
    {                          \
        int i_ = LOOPCOPY(a);  \
        do                     \
            (r)[i_] = (a)[i_]; \
        while (--i_ >= 0);     \
    }
#define INLINEset_clear(r, size) \
    {                            \
        int i_ = LOOPINIT(size); \
        *(r) = i_;               \
        do                       \
            (r)[i_] = 0;         \
        while (--i_ > 0);        \
    }
#define INLINEset_fill(r, size)                                \
    {                                                          \
        int i_ = LOOPINIT(size);                               \
        *(r) = i_;                                             \
        (r)[i_] = ((unsigned int)(~0)) >> (i_ * BPI - (size)); \
        while (--i_ > 0)                                       \
            (r)[i_] = ~0;                                      \
    }
#define INLINEset_and(r, a, b)           \
    {                                    \
        int i_ = LOOP(a);                \
        PUTLOOP(r, i_);                  \
        do                               \
            (r)[i_] = (a)[i_] & (b)[i_]; \
        while (--i_ > 0);                \
    }
#define INLINEset_or(r, a, b)            \
    {                                    \
        int i_ = LOOP(a);                \
        PUTLOOP(r, i_);                  \
        do                               \
            (r)[i_] = (a)[i_] | (b)[i_]; \
        while (--i_ > 0);                \
    }
#define INLINEset_diff(r, a, b)           \
    {                                     \
        int i_ = LOOP(a);                 \
        PUTLOOP(r, i_);                   \
        do                                \
            (r)[i_] = (a)[i_] & ~(b)[i_]; \
        while (--i_ > 0);                 \
    }
#define INLINEset_xor(r, a, b)           \
    {                                    \
        int i_ = LOOP(a);                \
        PUTLOOP(r, i_);                  \
        do                               \
            (r)[i_] = (a)[i_] ^ (b)[i_]; \
        while (--i_ > 0);                \
    }
#define INLINEset_merge(r, a, b, mask)                                  \
    {                                                                   \
        int i_ = LOOP(a);                                               \
        PUTLOOP(r, i_);                                                 \
        do                                                              \
            (r)[i_] = ((a)[i_] & (mask)[i_]) | ((b)[i_] & ~(mask)[i_]); \
        while (--i_ > 0);                                               \
    }
#define INLINEsetp_implies(a, b, when_false) \
    {                                        \
        int i_ = LOOP(a);                    \
        do                                   \
            if ((a)[i_] & ~(b)[i_])          \
                break;                       \
        while (--i_ > 0);                    \
        if (i_ != 0)                         \
            when_false;                      \
    }

#define count_ones(v)                                   \
    (bit_count[(v)&255] + bit_count[((v) >> 8) & 255] + \
     bit_count[((v) >> 16) & 255] + bit_count[((v) >> 24) & 255])

/* Table for efficient bit counting */
extern int bit_count[256];
/*----- END OF set.h ----- */

/* Define a boolean type */
#define bool int
#define FALSE 0
#define TRUE  1
#define MAYBE 2

/* Map many cube/cover types/routines into equivalent set types/routines */
#define pcube         pset
#define new_cube()    set_new(cube.size)
#define free_cube(r)  set_free(r)
#define pcover        pset_family
#define new_cover(i)  sf_new(i, cube.size)
#define free_cover(r) sf_free(r)
#define free_cubelist(T) \
    FREE((T)[0]);        \
    FREE(T);

/* cost_t describes the cost of a cover */
typedef struct cost_struct {
    int cubes;  /* number of cubes in the cover */
    int in;     /* transistor count, binary-valued variables */
    int out;    /* transistor count, output part */
    int mv;     /* transistor count, multiple-valued vars */
    int total;  /* total number of transistors */
    int primes; /* number of prime cubes */
} cost_t, *pcost;

/* PLA_t stores the logical representation of a PLA */
typedef struct {
    pcover F, D, R; /* on-set, off-set and dc-set */
} PLA_t, *pPLA;

typedef enum {
    TYPE_FD,
    TYPE_FR,
} pla_type_t;

#define equal(a, b) (strcmp(a, b) == 0)

/* This is a hack which I wish I hadn't done, but too painful to change */
#define CUBELISTSIZE(T) (((pcube *)(T)[1] - (T)) - 3)

/* For those who like to think about PLAs, macros to get at inputs/outputs */
#define GETINPUT(c, pos) \
    (((c)[WHICH_WORD(2 * (pos))] >> WHICH_BIT(2 * (pos))) & 3)

/*
 *  The cube structure is a global structure which contains information
 *  on how a set maps into a cube -- i.e., number of parts per variable,
 *  number of variables, etc.  Also, many fields are pre-computed to
 *  speed up various primitive operations.
 */
#define CUBE_TEMP 10

struct cube_struct {
    int size;            /* set size of a cube, number of columns */
    int num_input_vars;  /* number of binary variables */
    int *first_part;     /* first element of each variable */
    int *last_part;      /* first element of each variable */
    int *part_size;      /* number of elements in each variable */
    int *first_word;     /* first word for each variable */
    int *last_word;      /* last word for each variable */
    pset binary_mask;    /* Mask to extract binary variables */
    pset *var_mask;      /* mask to extract a variable */
    pset *temp;          /* an array of temporary sets */
    pset fullset;        /* a full cube */
    pset emptyset;       /* an empty cube */
    unsigned int inmask; /* mask to get odd word of binary part */
    int inword;          /* which word number for above */
    int output;          /* which variable is "output" (-1 if none) */
};

struct cdata_struct {
    int *part_zeros;   /* count of zeros for each element (column) */
    int *var_zeros;    /* count of zeros for each variable */
    int *parts_active; /* number of "active" parts for each var */
    bool *is_unate;    /* indicates given var is unate */
    int vars_active;   /* number of "active" variables */
    int vars_unate;    /* number of unate variables */
    int best;          /* best "binate" variable */
};

extern struct cube_struct cube;
extern struct cdata_struct cdata;

#define DISJOINT 0x55555555

/* function declarations */
/* cofactor.c */
pset *cofactor(pset *T, pset c);
pset *scofactor(pset *T, pset c, int var);
void massive_count(pset *T);
int binate_split_select(pset *T, pset cleft, pset cright);
pset *cube1list(pset_family A);
pset *cube2list(pset_family A, pset_family B);
pset *cube3list(pset_family A, pset_family B, pset_family C);
pset_family cubeunlist(pset *A1);
/* compl.c */
pset_family complement(pset *T);
/* contain.c */
pset_family sf_contain(pset_family A);
pset_family sf_rev_contain(pset_family A);
pset_family sf_dupl(pset_family A);
int rm_equal(pset *A1, int (*compare)(pset *, pset *));
int rm_contain(pset *A1);
int rm_rev_contain(pset *A1);
pset *sf_sort(pset_family A, int (*compare)(pset *, pset *));
pset *sf_list(pset_family A);
pset_family sf_unlist(pset *A1, int totcnt, int size);
pset_family d1merge(pset_family A, int var);
/* cubestr.c */
void cube_setup();
void setdown_cube();
/* cvrin.c */
void skip_line(FILE *fpin);
char *get_word(FILE *fp, char *word);
void read_cube(FILE *fp, pPLA PLA);
void parse_pla(FILE *fp, pPLA PLA);
int read_pla(FILE *fp, pPLA *PLA_return);
pPLA new_PLA();
void free_PLA(pPLA PLA);
/* cvrm.c */
pset_family unravel_output(pset_family B);
pset_family mini_sort(pset_family F, int (*compare)(pset *, pset *));
pset_family sort_reduce(pset_family T);
int cubelist_partition(pset *T, pset **A, pset **B);
/* cvrmisc.c */
void cover_cost(pset_family F, pcost cost);
void copy_cost(pcost s, pcost d);
void fatal(char *s);
/* cvrout.c */
void fprint_pla(FILE *fp, pPLA PLA);
void print_cube(FILE *fp, pset c, char *out_map);
/* espresso.c */
pset_family espresso(pset_family F, pset_family D1, pset_family R);
/* essen.c */
pset_family essential(pset_family *Fp, pset_family *Dp);
int essen_cube(pset_family F, pset_family D, pset c);
pset_family cb_consensus(pset_family T, pset c);
pset_family cb_consensus_dist0(pset_family R, pset p, pset c);
/* expand.c */
pset_family expand(pset_family F, pset_family R, int nonsparse);
void expand1(pset_family BB, pset_family CC, pset INIT_LOWER, pset c);
void essen_parts(pset_family BB, pset_family CC, pset RAISE, pset FREESET);
void essen_raising(pset_family BB, pset RAISE, pset FREESET);
void elim_lowering(pset_family BB, pset_family CC, pset RAISE, pset FREESET);
int most_frequent(pset_family CC, pset FREESET);
void select_feasible(pset_family BB, pset_family CC, pset RAISE, pset FREESET,
                     pset SUPER_CUBE, int *num_covered);
int feasibly_covered(pset_family BB, pset c, pset RAISE, pset new_lower);
void mincov(pset_family BB, pset RAISE, pset FREESET);
/* gasp.c */
pset_family expand_gasp(pset_family F, pset_family D, pset_family R,
                        pset_family Foriginal);
void expand1_gasp(pset_family F, pset_family D, pset_family R,
                  pset_family Foriginal, int c1index, pset_family *G);
pset_family irred_gasp(pset_family F, pset_family D, pset_family G);
pset_family last_gasp(pset_family F, pset_family D, pset_family R);
/* irred.c */
pset_family irredundant(pset_family F, pset_family D);
void mark_irredundant(pset_family F, pset_family D);
void irred_split_cover(pset_family F, pset_family D, pset_family *E,
                       pset_family *Rt, pset_family *Rp);
sm_matrix *irred_derive_table(pset_family D, pset_family E, pset_family Rp);
int cube_is_covered(pset *T, pset c);
int tautology(pset *T);
int taut_special_cases(pset *T);
/* reduce.c */
pset_family reduce(pset_family F, pset_family D);
pset reduce_cube(pset *FD, pset p);
pset sccc(pset *T);
pset sccc_merge(pset left, pset right, pset cl, pset cr);
pset sccc_cube(pset result, pset p);
int sccc_special_cases(pset *T, pset *result);
/* set.c */
int bit_index(unsigned int a);
int set_ord(pset a);
int set_dist(pset a, pset b);
pset set_clear(pset r, int size);
pset set_fill(pset r, int size);
pset set_copy(pset r, pset a);
pset set_and(pset r, pset a, pset b);
pset set_or(pset r, pset a, pset b);
pset set_diff(pset r, pset a, pset b);
pset set_merge(pset r, pset a, pset b, pset mask);
int setp_empty(pset a);
int setp_equal(pset a, pset b);
int setp_disjoint(pset a, pset b);
int setp_implies(pset a, pset b);
pset_family sf_active(pset_family A);
pset_family sf_inactive(pset_family A);
pset_family sf_copy(pset_family R, pset_family A);
pset_family sf_join(pset_family A, pset_family B);
pset_family sf_append(pset_family A, pset_family B);
pset_family sf_new(int num, int size);
pset_family sf_save(pset_family A);
void sf_free(pset_family A);
void sf_cleanup();
pset_family sf_addset(pset_family A, pset s);
void set_adjcnt(pset a, int *count, int weight);
int *sf_count(pset_family A);
int *sf_count_restricted(pset_family A, pset r);
/* setc.c */
int full_row(pset p, pset cof);
int cdist(pset a, pset b);
pset force_lower(pset xlower, pset a, pset b);
void consensus(pset r, pset a, pset b);
int cactive(pset a);
int ccommon(pset a, pset b, pset cof);
int descend(pset *a, pset *b);
int ascend(pset *a, pset *b);
int d1_order(pset *a, pset *b);
/* sminterf.c */
pset do_sm_minimum_cover(pset_family A);
/* sparse.c */
pset_family make_sparse(pset_family F, pset_family D, pset_family R);
pset_family mv_reduce(pset_family F, pset_family D);
/* unate.c */
pset_family map_cover_to_unate(pset *T);
pset_family map_unate_to_cover(pset_family A);
pset_family unate_compl(pset_family A);
pset_family unate_complement(pset_family A);

/* helper.c */
char *sf_print(pset_family A);
char *ps1(pset a);
