#include "sparse_int.h"

/*
 *  free-lists are only used if 'FAST_AND_LOOSE' is set; this is because
 *  we lose the debugging capability of libmm_t which trashes objects when
 *  they are free'd.  However, FAST_AND_LOOSE is much faster if matrices
 *  are created and freed frequently.
 */

#ifdef FAST_AND_LOOSE
sm_element *sm_element_freelist;
sm_row *sm_row_freelist;
sm_col *sm_col_freelist;
#endif

sm_matrix *sm_alloc() {
    sm_matrix *A;

    A = ALLOC(sm_matrix, 1);
    A->rows = NIL(sm_row *);
    A->cols = NIL(sm_col *);
    A->nrows = A->ncols = 0;
    A->rows_size = A->cols_size = 0;
    A->first_row = A->last_row = NIL(sm_row);
    A->first_col = A->last_col = NIL(sm_col);
    A->user_word = NIL(char); /* for our user ... */
    return A;
}

void sm_free(sm_matrix *A) {
#ifdef FAST_AND_LOOSE
    sm_row *prow;

    if (A->first_row != 0) {
        for (prow = A->first_row; prow != 0; prow = prow->next_row) {
            /* add the elements to the free list of elements */
            prow->last_col->next_col = sm_element_freelist;
            sm_element_freelist = prow->first_col;
        }

        /* Add the linked list of rows to the row-free-list */
        A->last_row->next_row = sm_row_freelist;
        sm_row_freelist = A->first_row;

        /* Add the linked list of cols to the col-free-list */
        A->last_col->next_col = sm_col_freelist;
        sm_col_freelist = A->first_col;
    }
#else
    sm_row *prow, *pnext_row;
    sm_col *pcol, *pnext_col;

    for (prow = A->first_row; prow != 0; prow = pnext_row) {
        pnext_row = prow->next_row;
        sm_row_free(prow);
    }
    for (pcol = A->first_col; pcol != 0; pcol = pnext_col) {
        pnext_col = pcol->next_col;
        pcol->first_row = pcol->last_row = NIL(sm_element);
        sm_col_free(pcol);
    }
#endif

    /* Free the arrays to map row/col numbers into pointers */
    FREE(A->rows);
    FREE(A->cols);
    FREE(A);
}

sm_matrix *sm_dup(sm_matrix *A) {
    sm_row *prow;
    sm_element *p;
    sm_matrix *B;

    B = sm_alloc();
    if (A->last_row != 0) {
        sm_resize(B, A->last_row->row_num, A->last_col->col_num);
        for (prow = A->first_row; prow != 0; prow = prow->next_row) {
            for (p = prow->first_col; p != 0; p = p->next_col) {
                (void)sm_insert(B, p->row_num, p->col_num);
            }
        }
    }
    return B;
}

void sm_resize(sm_matrix *A, int row, int col) {
    int i, new_size;

    if (row >= A->rows_size) {
        new_size = MAX(A->rows_size * 2, row + 1);
        A->rows = REALLOC(sm_row *, A->rows, new_size);
        for (i = A->rows_size; i < new_size; i++) {
            A->rows[i] = NIL(sm_row);
        }
        A->rows_size = new_size;
    }

    if (col >= A->cols_size) {
        new_size = MAX(A->cols_size * 2, col + 1);
        A->cols = REALLOC(sm_col *, A->cols, new_size);
        for (i = A->cols_size; i < new_size; i++) {
            A->cols[i] = NIL(sm_col);
        }
        A->cols_size = new_size;
    }
}

/*
 *  insert -- insert a value into the matrix
 */
sm_element *sm_insert(sm_matrix *A, int row, int col) {
    sm_row *prow;
    sm_col *pcol;
    sm_element *element;
    sm_element *save_element;

    if (row >= A->rows_size || col >= A->cols_size) {
        sm_resize(A, row, col);
    }

    prow = A->rows[row];
    if (prow == NIL(sm_row)) {
        prow = A->rows[row] = sm_row_alloc();
        prow->row_num = row;
        sorted_insert(sm_row, A->first_row, A->last_row, A->nrows, next_row,
                      prev_row, row_num, row, prow);
    }

    pcol = A->cols[col];
    if (pcol == NIL(sm_col)) {
        pcol = A->cols[col] = sm_col_alloc();
        pcol->col_num = col;
        sorted_insert(sm_col, A->first_col, A->last_col, A->ncols, next_col,
                      prev_col, col_num, col, pcol);
    }

    /* get a new item, save its address */
    sm_element_alloc(element);
    save_element = element;

    /* insert it into the row list */
    sorted_insert(sm_element, prow->first_col, prow->last_col, prow->length,
                  next_col, prev_col, col_num, col, element);

    /* if it was used, also insert it into the column list */
    if (element == save_element) {
        sorted_insert(sm_element, pcol->first_row, pcol->last_row, pcol->length,
                      next_row, prev_row, row_num, row, element);
    } else {
        /* otherwise, it was already in matrix -- free element we allocated */
        sm_element_free(save_element);
    }
    return element;
}

void sm_delrow(sm_matrix *A, int i) {
    sm_element *p, *pnext;
    sm_col *pcol;
    sm_row *prow;

    prow = sm_get_row(A, i);
    if (prow != NIL(sm_row)) {
        /* walk across the row */
        for (p = prow->first_col; p != 0; p = pnext) {
            pnext = p->next_col;

            /* unlink the item from the column (and delete it) */
            pcol = sm_get_col(A, p->col_num);
            sm_col_remove_element(pcol, p);

            /* discard the column if it is now empty */
            if (pcol->first_row == NIL(sm_element)) {
                sm_delcol(A, pcol->col_num);
            }
        }

        /* discard the row -- we already threw away the elements */
        A->rows[i] = NIL(sm_row);
        dll_unlink(prow, A->first_row, A->last_row, next_row, prev_row,
                   A->nrows);
        prow->first_col = prow->last_col = NIL(sm_element);
        sm_row_free(prow);
    }
}

void sm_delcol(sm_matrix *A, int i) {
    sm_element *p, *pnext;
    sm_row *prow;
    sm_col *pcol;

    pcol = sm_get_col(A, i);
    if (pcol != NIL(sm_col)) {
        /* walk down the column */
        for (p = pcol->first_row; p != 0; p = pnext) {
            pnext = p->next_row;

            /* unlink the element from the row (and delete it) */
            prow = sm_get_row(A, p->row_num);
            sm_row_remove_element(prow, p);

            /* discard the row if it is now empty */
            if (prow->first_col == NIL(sm_element)) {
                sm_delrow(A, prow->row_num);
            }
        }

        /* discard the column -- we already threw away the elements */
        A->cols[i] = NIL(sm_col);
        dll_unlink(pcol, A->first_col, A->last_col, next_col, prev_col,
                   A->ncols);
        pcol->first_row = pcol->last_row = NIL(sm_element);
        sm_col_free(pcol);
    }
}

void sm_cleanup() {
#ifdef FAST_AND_LOOSE
    sm_element *p, *pnext;
    sm_row *prow, *pnextrow;
    sm_col *pcol, *pnextcol;

    for (p = sm_element_freelist; p != 0; p = pnext) {
        pnext = p->next_col;
        FREE(p);
    }
    sm_element_freelist = 0;

    for (prow = sm_row_freelist; prow != 0; prow = pnextrow) {
        pnextrow = prow->next_row;
        FREE(prow);
    }
    sm_row_freelist = 0;

    for (pcol = sm_col_freelist; pcol != 0; pcol = pnextcol) {
        pnextcol = pcol->next_col;
        FREE(pcol);
    }
    sm_col_freelist = 0;
#endif
}
