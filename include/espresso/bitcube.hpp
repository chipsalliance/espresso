/**
 * @file bitcube.hpp
 * @brief Modern C++23 data structures for representing cubes and covers in logic minimization.
 *
 * Provides BitCube (fixed-width bitset for cubes), CubeList (vector of cubes),
 * and Cover (a set of cubes) for use in logic minimization algorithms.
 *
 * - BitCube: Efficient bitset abstraction for representing a single cube.
 * - CubeList: Alias for a vector of BitCubes.
 * - Cover: Container for a set of cubes, with convenient access and comparison.
 *
 */
#pragma once
#include <bitset>
#include <vector>
#include <cstdint>
#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <ostream>
#include <string>
#include <type_traits>

namespace espresso {

/**
 * @class BitCube
 * @brief Fixed-width bitset for representing a single logic cube.
 *
 * Provides bitwise operations, set/reset, and comparison for cube manipulation.
 */
template <size_t WIDTH>
class BitCube {
public:
    /**
     * @brief Default constructor (all bits zero).
     */
    BitCube();
    /**
     * @brief Construct a BitCube with specified bits set to 1.
     * @param ones Indices of bits to set.
     */
    BitCube(std::initializer_list<size_t> ones);
    /**
     * @brief Equality comparison.
     */
    bool operator==(const BitCube& other) const;
    /**
     * @brief Inequality comparison.
     */
    bool operator!=(const BitCube& other) const;
    /**
     * @brief Test if bit i is set.
     */
    bool test(size_t i) const;
    /**
     * @brief Set or clear bit i.
     * @param i Bit index
     * @param v Value to set (default: true)
     */
    void set(size_t i, bool v = true);
    /**
     * @brief Reset all bits to zero.
     */
    void reset();
    /**
     * @brief Count the number of bits set to 1.
     */
    size_t count() const;
    /**
     * @brief Get the total number of bits.
     */
    size_t size() const;
    /**
     * @brief Serialize BitCube to a string of '0' and '1'.
     */
    std::string to_string() const;
    /**
     * @brief Deserialize BitCube from a string of '0' and '1'.
     * @throws std::invalid_argument if the string is not valid.
     */
    static BitCube from_string(const std::string& s);
private:
    std::bitset<WIDTH> bits_;
};

/**
 * @brief List of BitCubes (alias for std::vector<BitCube<WIDTH>>).
 */
template <size_t WIDTH>
using CubeList = std::vector<BitCube<WIDTH>>;

/**
 * @class Cover
 * @brief Container for a set of cubes (the cover).
 *
 * Provides convenient access, iteration, and comparison for sets of cubes.
 */
template <size_t WIDTH>
class Cover {
public:
    /**
     * @brief Default constructor (empty cover).
     */
    Cover();
    /**
     * @brief Construct a cover from an initializer list of cubes.
     */
    Cover(std::initializer_list<BitCube<WIDTH>> cubes);
    /**
     * @brief Add a cube to the cover.
     */
    void add(const BitCube<WIDTH>& c);
    /**
     * @brief Get the number of cubes in the cover.
     */
    size_t size() const;
    /**
     * @brief Check if the cover is empty.
     */
    bool empty() const;
    /**
     * @brief Access a cube by index (const).
     */
    const BitCube<WIDTH>& operator[](size_t i) const;
    /**
     * @brief Access a cube by index (mutable).
     */
    BitCube<WIDTH>& operator[](size_t i);
    /**
     * @brief Iterator to beginning (mutable).
     */
    auto begin() -> typename CubeList<WIDTH>::iterator;
    /**
     * @brief Iterator to end (mutable).
     */
    auto end() -> typename CubeList<WIDTH>::iterator;
    /**
     * @brief Iterator to beginning (const).
     */
    auto begin() const -> typename CubeList<WIDTH>::const_iterator;
    /**
     * @brief Iterator to end (const).
     */
    auto end() const -> typename CubeList<WIDTH>::const_iterator;
    /**
     * @brief Equality comparison for covers.
     */
    bool operator==(const Cover<WIDTH>& other) const;
    /**
     * @brief Serialize Cover to a vector of strings (one per cube).
     */
    std::vector<std::string> to_strings() const;
    /**
     * @brief Deserialize Cover from a vector of strings.
     * @throws std::invalid_argument if any string is not valid.
     */
    static Cover<WIDTH> from_strings(const std::vector<std::string>& v);
    /**
     * @brief Clear all cubes from the cover.
     */
    void clear();
private:
    std::vector<BitCube<WIDTH>> cubes_;
};

} // namespace espresso

