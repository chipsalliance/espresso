//
// Created by yqszxx on 8/24/21.
//

#ifndef ESPRESSO_CUBE_HPP
#define ESPRESSO_CUBE_HPP

#include <memory>
#include <boost/dynamic_bitset.hpp>

class Cube {
public:
    using Element = size_t;
    using Container = boost::dynamic_bitset<>;
    explicit Cube(size_t size);
    Cube(const Cube &old);
    size_t size();
    void set(Element i);
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] bool test(Element i) const;

//private:
    std::unique_ptr<Container> data;

};


#endif //ESPRESSO_CUBE_HPP
