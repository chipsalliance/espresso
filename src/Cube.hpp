//
// Created by yqszxx on 8/24/21.
//

#ifndef ESPRESSO_CUBE_HPP
#define ESPRESSO_CUBE_HPP

#include <memory>
#include <boost/dynamic_bitset.hpp>

class Cube {
public:
    using Element = int;
    using Container = boost::dynamic_bitset<>;
    explicit Cube(size_t size);
    size_t size();

private:
    std::unique_ptr<Container> data;

};


#endif //ESPRESSO_CUBE_HPP
