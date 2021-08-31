//
// Created by yqszxx on 8/24/21.
//

#include "Cube.hpp"

Cube::Cube(size_t size) : data(std::make_unique<Container>(size)) {
}

size_t Cube::size() {
    return data->size();
}

void Cube::set(Cube::Element i) {
    data->set(i);
}

Cube::Cube(const Cube &old) : data(std::make_unique<Container>(*(old.data))) {

}

std::string Cube::toString() const {
    std::string s;
    boost::to_string(*(data), s);
    return s;
}

bool Cube::test(Cube::Element i) const {
    return data->test(i);
}

