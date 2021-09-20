//
// Created by yqszxx on 8/24/21.
//

#ifndef ESPRESSO_CUBE_HPP
#define ESPRESSO_CUBE_HPP

#include <memory>
#include <boost/dynamic_bitset.hpp>

namespace espresso {

class Cube {
public:
    using Container = boost::dynamic_bitset<>;

    explicit Cube();
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
    std::unique_ptr<Container> data;

};

} // namespace espresso

#endif //ESPRESSO_CUBE_HPP
