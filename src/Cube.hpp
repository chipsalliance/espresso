//
// Created by yqszxx on 8/24/21.
//

#ifndef ESPRESSO_CUBE_HPP
#define ESPRESSO_CUBE_HPP

#include <memory>
#include <boost/dynamic_bitset.hpp>
#include "BitPat.hpp"

namespace espresso {

class Cube {
public:
    using Element = size_t;
    using Container = BitPat;

    explicit Cube();
    explicit Cube(std::string&);
    Cube(const Cube &old);
    [[nodiscard]] std::string toString(char on, char off) const;

    static void setInputCount(size_t i);
    static void setOutputCount(size_t o);

    static size_t getInputCount();
    static size_t getOutputCount();

    static std::tuple<
        std::shared_ptr<Cube>,  // f
        std::shared_ptr<Cube>,  // d
        std::shared_ptr<Cube>   // r
    > parse(std::string s);

private:
    static size_t inputCount, outputCount;
    std::unique_ptr<Container> input, output;

};

} // namespace espresso

#endif //ESPRESSO_CUBE_HPP
