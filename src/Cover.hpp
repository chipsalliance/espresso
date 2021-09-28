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

    unsigned int activeCount{};

    explicit Cover();
    void insert(const Element& e);
    [[nodiscard]] std::string toString(char on, char off) const;
    void miniSort(bool ascend);
    Cover& operator = (const Cover& x);

    Container cubes;
};

} // namespace espresso

#endif // ESPRESSO_COVER_HPP
