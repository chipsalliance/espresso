#include <stdio.h>
#include <stdlib.h>
#include "sparse.h"
#include "mincov.h"


#define NIL(type)        ((type *)0)
#define ALLOC(type, num) ((type *)malloc(sizeof(type) * (num)))
#define REALLOC(type, obj, num)                                    \
    (obj) ? ((type *)realloc((char *)(obj), sizeof(type) * (num))) \
          : ((type *)malloc(sizeof(type) * (num)))
#define FREE(obj)                  \
    if ((obj)) {                   \
        (void)free((char *)(obj)); \
        (obj) = 0;                 \
    }

#define MAX(a, b) ((a) > (b) ? (a) : (b))


typedef struct stats_struct stats_t;
struct stats_struct {
    int max_depth;    /* deepest the recursion has gone */
    int nodes;        /* total nodes visited */
    int component;    /* currently solving a component */
    int comp_count;   /* number of components detected */
    int gimpel_count; /* number of times Gimpel reduction applied */
    int gimpel;       /* currently inside Gimpel reduction */
    int no_branching;
};

typedef struct solution_struct solution_t;
struct solution_struct {
    sm_row *row;
    int cost;
};

/* mincov.c */
sm_row *sm_minimum_cover(sm_matrix *A, int *weight, int heuristic);
solution_t *sm_mincov(sm_matrix *A, solution_t *select, int *weight, int lb,
                      int bound, int depth, stats_t *stats);
/* solution.c */
solution_t *solution_alloc();
void solution_free(solution_t *sol);
solution_t *solution_dup(solution_t *sol);
void solution_add(solution_t *sol, int *weight, int col);
void solution_accept(solution_t *sol, sm_matrix *A, int *weight, int col);
void solution_reject(solution_t *sol, sm_matrix *A, int *weight, int col);
solution_t *solution_choose_best(solution_t *best1, solution_t *best2);

/* indep.c */
solution_t *sm_maximal_independent_set(sm_matrix *A, int *weight);

/* gimpel.c */
int gimpel_reduce(sm_matrix *A, solution_t *select, int *weight, int lb,
                  int bound, int depth, stats_t *stats, solution_t **best);

#define WEIGHT(weight, col) (weight == NIL(int) ? 1 : weight[col])
