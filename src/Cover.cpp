//
// Created by yqszxx on 8/24/21.
//

#include <iostream>
#include <algorithm>
#include "Cover.hpp"

espresso::Cover::Cover() : activeCount(0) {};

void espresso::Cover::insert(const espresso::Cover::Element& e) {
    cubes.push_back(e);
}

std::string espresso::Cover::toString(char on, char off) const {
    std::string s;
    for (const auto& e: cubes) {
        s += e->toString(on, off) + "\n";
    }
    return s;
}

void espresso::Cover::miniSort(bool ascend) {
    std::vector<unsigned int> columnSum(Cube::getCount(), 0);
    for (auto i = 0; i < Cube::getCount(); i++) {
        for (const auto& c: cubes) {
            if (c->test(i)) columnSum[i]++;
        }
    }

    // rank is the "inner product of the cube and the column sums"
    for (const auto& c: cubes) {
        c->rank = 0;
        for (auto i = 0; i < Cube::getCount(); i++) {
            if (c->test(i)) c->rank += columnSum[i];
        }
    }

    std::sort(cubes.begin(), cubes.end(), [&ascend](const auto& a, const auto& b) {
            return ascend ? a->rank < b->rank : a->rank > b->rank;
    });
}

espresso::Cover &espresso::Cover::operator=(const espresso::Cover &x) {
    cubes = x.cubes;
    return *this;
}
