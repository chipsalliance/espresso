//
// Created by yqszxx on 8/24/21.
//

#include "Cube.hpp"

Cube::Cube(size_t size) :
    data(std::make_unique<Container>(size))
{
}

size_t Cube::size() {
    return data->size();
}
