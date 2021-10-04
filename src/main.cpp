//
// Created by yqszxx on 8/24/21.
//

#include "Pla.hpp"
#include <iostream>

int main(int, char*[]) {
    espresso::PLA pla;
    espresso::Cost cost = pla.cost();
    std::cerr << "I:c=" << cost.cubes << ",in=" << cost.in << ",out=" << cost.out << ",total=" << cost.in + cost.out << std::endl;
    pla.expand();
    pla.dump();
    cost = pla.cost();
    std::cerr << "O:c=" << cost.cubes << ",in=" << cost.in << ",out=" << cost.out << ",total=" << cost.in + cost.out << std::endl;
    return EXIT_SUCCESS;
}
