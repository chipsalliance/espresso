//
// Created by yqszxx on 8/24/21.
//

#ifndef ESPRESSO_COVER_HPP
#define ESPRESSO_COVER_HPP

#include <vector>
#include <memory>
#include "Cube.hpp"

namespace espresso {

class Cover {
public:
    using Element = std::shared_ptr<Cube>;
    using Container = std::vector<Element>;

    explicit Cover(size_t cubeSize);
    void insert(const Element& e) const;

//private:
    std::unique_ptr<Container> data;
    size_t cubeSize;
};

} // namespace espresso

#endif // ESPRESSO_COVER_HPP
