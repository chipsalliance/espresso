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

    explicit Cover();
    void insert(const Element& e);
    [[nodiscard]] std::string toString(char on, char off) const;

private:
    Container data;
};

} // namespace espresso

#endif // ESPRESSO_COVER_HPP
