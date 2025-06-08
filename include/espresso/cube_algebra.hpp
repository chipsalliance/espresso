/**
 * @file cube_algebra.hpp
 * @brief Modern C++23 cube algebra operations for logic minimization.
 *
 * This module provides cube distance calculations, consensus operations,
 * cofactoring, and other algebraic operations on BitCubes and Covers.
 * These operations are modernized versions of the legacy setc.c and cofactor.c functions.
 */
#pragma once

#include <espresso/bitcube.hpp>
#include <concepts>
#include <span>
#include <optional>
#include <ranges>
#include <algorithm>
#include <cstdint>

namespace espresso {

/**
 * @concept BitCubeLike
 * @brief Concept for types that behave like BitCube (for generic algorithms).
 */
template<typename T>
concept BitCubeLike = requires(T t, size_t i) {
    { t.test(i) } -> std::convertible_to<bool>;
    { t.size() } -> std::convertible_to<size_t>;
    { t.count() } -> std::convertible_to<size_t>;
};

/**
 * @concept CoverLike
 * @brief Concept for types that behave like Cover (for generic algorithms).
 */
template<typename T>
concept CoverLike = requires(T t, size_t i) {
    { t[i] } -> BitCubeLike;
    { t.size() } -> std::convertible_to<size_t>;
    { t.begin() } -> std::forward_iterator;
    { t.end() } -> std::forward_iterator;
};

namespace cube_algebra {

/**
 * @brief Calculate the distance between two cubes.
 * 
 * Distance is defined as the number of variables where the cubes are disjoint
 * (i.e., have no common minterms).
 * 
 * @param a First cube
 * @param b Second cube  
 * @return Number of disjoint variables between a and b
 */
template<size_t WIDTH>
[[nodiscard]] int distance(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Check if two cubes are distance 0 apart (intersect).
 * 
 * Two cubes are distance 0 if they share at least one minterm in every variable.
 * This is an optimized version for the common case where only intersection is needed.
 * 
 * @param a First cube
 * @param b Second cube
 * @return true if cubes intersect (distance 0), false otherwise
 */
template<size_t WIDTH>
[[nodiscard]] bool distance_zero(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Calculate distance with early termination for values > 1.
 * 
 * Returns actual distance if ≤ 1, otherwise returns 2 for efficiency.
 * Useful for distance-1 merge operations.
 * 
 * @param a First cube
 * @param b Second cube
 * @return Distance (0, 1, or 2 if > 1)
 */
template<size_t WIDTH>
[[nodiscard]] int distance_01(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Compute the consensus of two cubes that are distance 1 apart.
 * 
 * The consensus is formed by taking the intersection where cubes agree,
 * and the union where they disagree (for the single disagreeing variable).
 * 
 * @param a First cube (must be distance 1 from b)
 * @param b Second cube (must be distance 1 from a)
 * @return Consensus cube, or nullopt if cubes are not distance 1
 */
template<size_t WIDTH>
[[nodiscard]] std::optional<BitCube<WIDTH>> consensus(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Compute cofactor of a cube against another cube.
 * 
 * The cofactor restricts the cube to the minterms of the cofactoring cube.
 * Returns nullopt if the cubes are disjoint.
 * 
 * @param cube Cube to be cofactored
 * @param cofactor_cube Cube to cofactor against
 * @return Cofactored cube, or nullopt if disjoint
 */
template<size_t WIDTH>
[[nodiscard]] std::optional<BitCube<WIDTH>> cofactor_cube(const BitCube<WIDTH>& cube, const BitCube<WIDTH>& cofactor_cube) noexcept;

/**
 * @brief Compute cofactor of a cover against a cube.
 * 
 * Returns a new cover containing the cofactor of each cube in the input cover
 * that intersects with the cofactoring cube.
 * 
 * @param cover Cover to be cofactored
 * @param cofactor Cube to cofactor against
 * @return New cover containing cofactored cubes
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> cofactor_cover(const Cover<WIDTH>& cover, const BitCube<WIDTH>& cofactor);

/**
 * @brief Check if a cube is "active" in exactly one variable.
 * 
 * A cube is active in a variable if it doesn't cover all minterms of that variable.
 * Returns the index of the single active variable, or nullopt if there are 0 or >1.
 * 
 * @param cube Cube to check
 * @return Index of single active variable, or nullopt
 */
template<size_t WIDTH>
[[nodiscard]] std::optional<size_t> single_active_variable(const BitCube<WIDTH>& cube) noexcept;

/**
 * @brief Check if two cubes share any active variables.
 * 
 * Active variables are those where a cube doesn't cover all minterms.
 * This is useful for determining compatibility in various algorithms.
 * 
 * @param a First cube
 * @param b Second cube
 * @param mask Optional mask to restrict which variables to consider
 * @return true if cubes share active variables
 */
template<size_t WIDTH>
[[nodiscard]] bool share_active_variables(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b, 
                                         const BitCube<WIDTH>& mask = BitCube<WIDTH>{}) noexcept;

/**
 * @brief Determine which variables of cube a do not intersect cube b.
 * 
 * Used in expansion algorithms to identify variables that can be expanded.
 * 
 * @param a First cube
 * @param b Second cube
 * @return BitCube marking disjoint variables
 */
template<size_t WIDTH>
[[nodiscard]] BitCube<WIDTH> disjoint_variables(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Check if a cube covers all minterms in certain variables.
 * 
 * Used to determine if a cube is "full" with respect to a cofactor.
 * 
 * @param cube Cube to check
 * @param cofactor Cofactor defining the context
 * @return true if cube is full row with respect to cofactor
 */
template<size_t WIDTH>
[[nodiscard]] bool is_full_row(const BitCube<WIDTH>& cube, const BitCube<WIDTH>& cofactor) noexcept;

/**
 * @brief Determine which variables of cube a do not intersect cube b.
 * 
 * This is the modernized version of force_lower() from legacy setc.c.
 * Used in expansion algorithms to identify variables that can be expanded.
 * 
 * @param a First cube  
 * @param b Second cube
 * @return BitCube with bits set for variables where a and b are disjoint
 */
template<size_t WIDTH>
[[nodiscard]] BitCube<WIDTH> force_lower(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Check if one cube contains (implies) another cube.
 * 
 * Cube a contains cube b if every minterm covered by b is also covered by a.
 * 
 * @param a Containing cube
 * @param b Contained cube
 * @return true if a contains b
 */
template<size_t WIDTH>
[[nodiscard]] bool cube_contains(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept;

/**
 * @brief Compute the complement of a single cube.
 * 
 * Returns a cover representing all minterms NOT covered by the input cube.
 * This implements De Morgan's law for a single cube.
 * 
 * @param cube Input cube to complement
 * @return Cover representing the complement
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> cube_complement(const BitCube<WIDTH>& cube);

/**
 * @brief Union two covers (OR operation).
 * 
 * @param a First cover
 * @param b Second cover  
 * @return Cover containing all cubes from both inputs
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> cover_union(const Cover<WIDTH>& a, const Cover<WIDTH>& b);

/**
 * @brief Check if one cover contains another cover.
 * 
 * Cover a contains cover b if every cube in b is contained by some cube in a.
 * 
 * @param a Containing cover
 * @param b Contained cover
 * @return true if a contains b
 */
template<size_t WIDTH>
[[nodiscard]] bool cover_contains(const Cover<WIDTH>& a, const Cover<WIDTH>& b) noexcept;

/**
 * @brief Remove duplicate cubes from a cover.
 * 
 * @param cover Input cover (may contain duplicates)
 * @return Cover with duplicates removed
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> remove_duplicates(const Cover<WIDTH>& cover);

/**
 * @brief Remove cubes that are contained by other cubes in the cover.
 * 
 * @param cover Input cover
 * @return Cover with contained cubes removed
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> remove_contained(const Cover<WIDTH>& cover);

namespace detail {
    /**
     * @brief Helper to count set bits efficiently using built-in popcount.
     */
    [[nodiscard]] constexpr inline size_t popcount(std::uint64_t x) noexcept {
        return __builtin_popcountll(x);
    }

    /**
     * @brief Helper to find the index of the least significant set bit.
     */
    [[nodiscard]] constexpr inline size_t ctz(std::uint64_t x) noexcept {
        return x ? __builtin_ctzll(x) : 64;
    }
}

} // namespace cube_algebra
} // namespace espresso

