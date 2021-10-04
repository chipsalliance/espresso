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
    void dump() const;
    void expand();

private:
    Cover F, D, R;

    void parse();
};

} // namespace espresso


#endif //ESPRESSO_PLA_HPP
