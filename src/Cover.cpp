//
// Created by yqszxx on 8/24/21.
//

#include "Cover.hpp"

espresso::Cover::Cover(size_t cubeSize) : cubeSize(cubeSize) {

}

void espresso::Cover::insert(const espresso::Cover::Element& e) {
    assert(e->size() == cubeSize);
    data.push_back(e);
}
