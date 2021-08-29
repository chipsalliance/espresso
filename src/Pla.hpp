//
// Created by yqszxx on 8/29/21.
//

#ifndef ESPRESSO_PLA_HPP
#define ESPRESSO_PLA_HPP

#include "Cover.hpp"

namespace espresso {

class PLA {
public:
    explicit PLA();
private:
    std::unique_ptr<Cover> F, D, R;
    size_t inputCount, outputCount;

    void parse();
};

} // namespace espresso


#endif //ESPRESSO_PLA_HPP
