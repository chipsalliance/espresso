//
// Created by yqszxx on 8/24/21.
//

#include <iostream>
#include <algorithm>
#include "Cover.hpp"

espresso::Cover::Cover() = default;

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
