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

    unsigned int rank = 0;

    // flags
    bool isCovered;
    bool isNonessential;
    bool isPrime;
    bool isActive;

    explicit Cube();
    Cube(const Cube &old);
    Cube& operator = (const Cube& x);
    Cube operator - (const Cube& x) const;
    Cube& operator -= (const Cube& x);
    Cube operator + (const Cube& x) const;
    Cube& operator += (const Cube& x);
    Cube operator & (const Cube& x) const;
    Cube operator ~ () const;
    bool operator <= (const Cube& x) const; // implies
    bool operator <=> (const Cube& x) const; // disjoint
    bool operator == (const Cube& x) const;
    bool operator != (const Cube& x) const;
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] std::string toString(char on, char off) const;
    [[nodiscard]] bool test(size_t pos) const;
    void set(size_t pos);
    void reset(size_t pos);
    void reset();
    [[nodiscard]] size_t count() const;
    [[nodiscard]] unsigned int cdist(const Cube& x) const;
    [[nodiscard]] unsigned int dist(const Cube& x) const;
    [[nodiscard]] bool contains(const Cube& x) const;
    void forceLower(Cube& xLower, const Cube& x) const;

    static void setInputCount(size_t i);
    static void setOutputCount(size_t o);

    static size_t getInputCount();
    static size_t getOutputCount();
    static size_t getCount();

    static Cube& getOutputMask();
    static Cube& getInputMask();
    static Cube& getFullSet();

    static std::tuple<
        std::shared_ptr<Cube>,  // f
        std::shared_ptr<Cube>,  // d
        std::shared_ptr<Cube>   // r
    > parse(std::string s);

private:
    static size_t inputCount, outputCount;
    static std::unique_ptr<Cube> inputMask, outputMask, fullSet;
    std::unique_ptr<Container> data;

};

} // namespace espresso

#endif //ESPRESSO_CUBE_HPP
