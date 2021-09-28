//
// Created by yqszxx on 9/27/21.
//

#include "./SparseMatrix/mincov.h"
#include "Cover.hpp"
#include "Cube.hpp"

using namespace espresso;

Cube SMMincov(Cover& A) {
    sm_matrix *M;
    sm_row *sparse_cover;
    sm_element *pe;
    int i, base, rownum;
    unsigned val;

    M = sm_alloc();
    rownum = 0;

    for (auto& c: A.cubes) {
        for (auto i = 0; i < Cube::getCount(); i++) {
            if (c->test(i)) {
                sm_insert(M, rownum, i);
            }
        }
        rownum++;
    }

#define NIL(type)        ((type *)0)
    sparse_cover = sm_minimum_cover(M, NIL(int), 1);
    sm_free(M);

    Cube c;
    sm_foreach_row_element(sparse_cover, pe) {
        c.set(pe->col_num);
    }
    sm_row_free(sparse_cover);

    return c;
}