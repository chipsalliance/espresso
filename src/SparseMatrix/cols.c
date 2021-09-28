#include "sparse_int.h"

/*
 *  allocate a new col vector
 */
sm_col *sm_col_alloc() {
    sm_col *pcol;

#ifdef FAST_AND_LOOSE
    if (sm_col_freelist == NIL(sm_col)) {
        pcol = ALLOC(sm_col, 1);
    } else {
        pcol = sm_col_freelist;
        sm_col_freelist = pcol->next_col;
    }
#else
    pcol = ALLOC(sm_col, 1);
#endif

    pcol->col_num = 0;
    pcol->length = 0;
    pcol->first_row = pcol->last_row = NIL(sm_element);
    pcol->next_col = pcol->prev_col = NIL(sm_col);
    pcol->flag = 0;
    pcol->user_word = NIL(char); /* for our user ... */
    return pcol;
}

/*
 *  free a col vector -- for FAST_AND_LOOSE, this is real cheap for cols;
 *  however, freeing a rowumn must still walk down the rowumn discarding
 *  the elements one-by-one; that is the only use for the extra '-DCOLS'
 *  compile flag ...
 */
void sm_col_free(sm_col *pcol) {
#if defined(FAST_AND_LOOSE) && !defined(COLS)
    if (pcol->first_row != NIL(sm_element)) {
        /* Add the linked list of col items to the free list */
        pcol->last_row->next_row = sm_element_freelist;
        sm_element_freelist = pcol->first_row;
    }

    /* Add the col to the free list of cols */
    pcol->next_col = sm_col_freelist;
    sm_col_freelist = pcol;
#else
    sm_element *p, *pnext;

    for (p = pcol->first_row; p != 0; p = pnext) {
        pnext = p->next_row;
        sm_element_free(p);
    }
    FREE(pcol);
#endif
}

/*
 *  return 1 if col p2 contains col p1; 0 otherwise
 */
int sm_col_contains(sm_col *p1, sm_col *p2) {
    sm_element *q1, *q2;

    q1 = p1->first_row;
    q2 = p2->first_row;
    while (q1 != 0) {
        if (q2 == 0 || q1->row_num < q2->row_num) {
            return 0;
        } else if (q1->row_num == q2->row_num) {
            q1 = q1->next_row;
            q2 = q2->next_row;
        } else {
            q2 = q2->next_row;
        }
    }
    return 1;
}

/*
 *  remove an element from a col vector (given a pointer to the element)
 */
void sm_col_remove_element(sm_col *pcol, sm_element *p) {
    dll_unlink(p, pcol->first_row, pcol->last_row, next_row, prev_row,
               pcol->length);
    sm_element_free(p);
}
