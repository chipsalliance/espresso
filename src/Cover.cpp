//
// Created by yqszxx on 8/24/21.
//

#include "Cover.hpp"

espresso::Cover::Cover() {

}

void espresso::Cover::insert(const espresso::Cover::Element& e) {
    data.push_back(e);
}

std::string espresso::Cover::toString(char on, char off) const {
    std::string s;
    for (const auto& e: data) {
        s += e->toString(on, off) + "\n";
    }
    return s;
}
