/**
 * @file cube_algebra.cpp
 * @brief Implementation of modern C++23 cube algebra operations.
 *
 * This file implements the cube distance, consensus, cofactor, and other algebraic
 * operations that were originally in setc.c and cofactor.c, but modernized for C++23.
 */

#include <espresso/cube_algebra.hpp>
#include <ranges>
#include <algorithm>
#include <bit>

namespace espresso::cube_algebra {

namespace {
    /**
     * @brief Extract specific bits from a bitset for variable analysis.
     * 
     * In the original Espresso, variables could be binary (1 bit) or multi-valued (>1 bit).
     * For simplicity in this modernized version, we'll treat each bit as a binary variable,
     * but keep the structure flexible for future extensions.
     */
    template<size_t WIDTH>
    constexpr bool bits_intersect(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b, size_t start, size_t len) noexcept {
        for (size_t i = start; i < start + len && i < WIDTH; ++i) {
            if (a.test(i) && b.test(i)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Count disjoint variables in a range of bits.
     */
    template<size_t WIDTH>
    constexpr size_t count_disjoint_in_range(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b, size_t start, size_t len) noexcept {
        size_t disjoint_count = 0;
        for (size_t i = start; i < start + len && i < WIDTH; ++i) {
            if (!(a.test(i) && b.test(i))) {
                ++disjoint_count;
            }
        }
        return disjoint_count;
    }
}

template<size_t WIDTH>
int distance(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    // Distance is the number of bit positions where the cubes differ
    int dist = 0;
    for (size_t i = 0; i < WIDTH; ++i) {
        if (a.test(i) != b.test(i)) {
            ++dist;
        }
    }
    return dist;
}

template<size_t WIDTH>
bool distance_zero(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    // Two cubes have distance zero if they are identical
    return a == b;
}

template<size_t WIDTH>
int distance_01(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    int dist = 0;
    
    for (size_t i = 0; i < WIDTH; ++i) {
        if (a.test(i) != b.test(i)) {
            ++dist;
            if (dist > 1) {
                return 2; // Early termination for efficiency
            }
        }
    }
    
    return dist;
}

template<size_t WIDTH>
std::optional<BitCube<WIDTH>> consensus(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    // Check if cubes are identical (distance 0) or distance 1 apart
    int dist = distance_01(a, b);
    if (dist != 0 && dist != 1) {
        return std::nullopt;
    }
    
    // For identical cubes (distance 0), return either cube
    if (dist == 0) {
        return a;
    }
    
    // For distance-1 cubes, consensus is:
    // - Intersection where cubes agree  
    // - Union where they disagree (should be exactly one variable)
    BitCube<WIDTH> result;
    
    for (size_t i = 0; i < WIDTH; ++i) {
        if (a.test(i) == b.test(i)) {
            // Cubes agree - take intersection (which is the same as either)
            result.set(i, a.test(i));
        } else {
            // Cubes disagree - take union (both bits set)
            result.set(i, true);
        }
    }
    
    return result;
}

template<size_t WIDTH>
std::optional<BitCube<WIDTH>> cofactor_cube(const BitCube<WIDTH>& cube, const BitCube<WIDTH>& cofactor_cube) noexcept {
    // Check if cubes intersect (distance 0)
    if (!distance_zero(cube, cofactor_cube)) {
        return std::nullopt; // Disjoint cubes have no cofactor
    }
    
    // Cofactor is the intersection of the cube restricted to the cofactor
    BitCube<WIDTH> result;
    
    for (size_t i = 0; i < WIDTH; ++i) {
        // The cofactor contains the intersection
        result.set(i, cube.test(i) && cofactor_cube.test(i));
    }
    
    return result;
}

template<size_t WIDTH>
Cover<WIDTH> cofactor_cover(const Cover<WIDTH>& cover, const BitCube<WIDTH>& cofactor) {
    Cover<WIDTH> result;
    
    // Apply cofactor operation to each cube in the cover
    for (const auto& cube : cover) {
        if (auto cofactored = cofactor_cube<WIDTH>(cube, cofactor)) {
            result.add(*cofactored);
        }
        // Skip cubes that are disjoint with the cofactor
    }
    
    return result;
}

template<size_t WIDTH>
std::optional<size_t> single_active_variable(const BitCube<WIDTH>& cube) noexcept {
    std::optional<size_t> active_var = std::nullopt;
    size_t active_count = 0;
    
    // In the simplified model, a variable is "active" if the corresponding bit is not set
    // (meaning the cube doesn't cover all minterms of that variable)
    
    for (size_t i = 0; i < WIDTH; ++i) {
        if (!cube.test(i)) { // Variable is active (not fully covered)
            active_var = i;
            ++active_count;
            if (active_count > 1) {
                return std::nullopt; // More than one active variable
            }
        }
    }
    
    return active_count == 1 ? active_var : std::nullopt;
}

template<size_t WIDTH>
bool share_active_variables(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b, const BitCube<WIDTH>& mask) noexcept {
    // Check if both cubes have active variables in common positions
    
    for (size_t i = 0; i < WIDTH; ++i) {
        // Skip masked variables
        if (mask.size() > 0 && mask.test(i)) {
            continue;
        }
        
        // Both cubes are active in this variable
        if (!a.test(i) && !b.test(i)) {
            return true;
        }
    }
    
    return false;
}

template<size_t WIDTH>
BitCube<WIDTH> disjoint_variables(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    BitCube<WIDTH> result;
    
    // Mark variables where cubes are disjoint
    for (size_t i = 0; i < WIDTH; ++i) {
        // Variables are disjoint if cubes don't intersect in that variable
        if (!(a.test(i) && b.test(i))) {
            result.set(i, true);
        }
    }
    
    return result;
}

template<size_t WIDTH>
bool is_full_row(const BitCube<WIDTH>& cube, const BitCube<WIDTH>& cofactor) noexcept {
    // A cube is a "full row" if it covers all minterms not excluded by the cofactor
    
    for (size_t i = 0; i < WIDTH; ++i) {
        // If cofactor allows this variable but cube doesn't cover it
        if (cofactor.test(i) && !cube.test(i)) {
            return false;
        }
    }
    
    return true;
}

template<size_t WIDTH>
BitCube<WIDTH> force_lower(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    // Same as disjoint_variables - returns variables where a and b don't intersect
    return disjoint_variables(a, b);
}

template<size_t WIDTH>
bool cube_contains(const BitCube<WIDTH>& a, const BitCube<WIDTH>& b) noexcept {
    // Cube a contains cube b if every bit set in b is also set in a
    // In our representation, this means a has at least the coverage of b
    
    for (size_t i = 0; i < WIDTH; ++i) {
        // If b covers a variable but a doesn't, then a doesn't contain b
        if (b.test(i) && !a.test(i)) {
            return false;
        }
    }
    
    return true;
}

template<size_t WIDTH>
Cover<WIDTH> cube_complement(const BitCube<WIDTH>& cube) {
    Cover<WIDTH> result;
    
    // For each variable where the cube has a 1 (is set/covers that variable),
    // create a complement cube with that variable unset (0) and all others don't care
    
    for (size_t i = 0; i < WIDTH; ++i) {
        if (cube.test(i)) {
            BitCube<WIDTH> complement_cube;
            // Copy all bits except the i-th one
            for (size_t j = 0; j < WIDTH; ++j) {
                if (i != j) {
                    complement_cube.set(j, cube.test(j));
                }
                // Leave bit i as 0 (unset) to create the complement
            }
            result.add(complement_cube);
        }
    }
    
    return result;
}

template<size_t WIDTH>
Cover<WIDTH> cover_union(const Cover<WIDTH>& a, const Cover<WIDTH>& b) {
    Cover<WIDTH> result = a; // Start with a copy of cover a
    
    // Add all cubes from cover b
    for (const auto& cube : b) {
        result.add(cube);
    }
    
    return result;
}

template<size_t WIDTH>
bool cover_contains(const Cover<WIDTH>& a, const Cover<WIDTH>& b) noexcept {
    // Cover a contains cover b if every cube in b is contained by some cube in a
    
    for (const auto& cube_b : b) {
        bool found_container = false;
        
        for (const auto& cube_a : a) {
            if (cube_contains(cube_a, cube_b)) {
                found_container = true;
                break;
            }
        }
        
        if (!found_container) {
            return false; // cube_b is not contained by any cube in a
        }
    }
    
    return true;
}

template<size_t WIDTH>
Cover<WIDTH> remove_duplicates(const Cover<WIDTH>& cover) {
    Cover<WIDTH> result;
    
    for (const auto& cube : cover) {
        // Check if this cube is already in the result
        bool is_duplicate = false;
        for (const auto& existing : result) {
            if (cube == existing) {
                is_duplicate = true;
                break;
            }
        }
        
        if (!is_duplicate) {
            result.add(cube);
        }
    }
    
    return result;
}

template<size_t WIDTH>
Cover<WIDTH> remove_contained(const Cover<WIDTH>& cover) {
    Cover<WIDTH> result;
    
    for (const auto& cube : cover) {
        bool is_contained = false;
        
        // Check if this cube is contained by any other cube in the cover
        for (const auto& other : cover) {
            if (cube != other && cube_contains(other, cube)) {
                is_contained = true;
                break;
            }
        }
        
        if (!is_contained) {
            result.add(cube);
        }
    }
    
    return result;
}


// Explicit template instantiations for commonly used sizes
template int distance<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template int distance<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template int distance<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template int distance<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template int distance<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template int distance<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template int distance<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template int distance<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template int distance<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template int distance<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template int distance<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template bool distance_zero<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template bool distance_zero<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template bool distance_zero<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template bool distance_zero<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template bool distance_zero<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template bool distance_zero<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template bool distance_zero<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template bool distance_zero<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template bool distance_zero<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template bool distance_zero<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template bool distance_zero<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template int distance_01<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template int distance_01<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template int distance_01<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template int distance_01<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template int distance_01<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template int distance_01<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template int distance_01<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template int distance_01<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template int distance_01<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template int distance_01<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template int distance_01<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template std::optional<BitCube<1>> consensus<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template std::optional<BitCube<2>> consensus<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template std::optional<BitCube<4>> consensus<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template std::optional<BitCube<8>> consensus<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template std::optional<BitCube<16>> consensus<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template std::optional<BitCube<32>> consensus<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template std::optional<BitCube<64>> consensus<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template std::optional<BitCube<128>> consensus<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template std::optional<BitCube<256>> consensus<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template std::optional<BitCube<512>> consensus<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template std::optional<BitCube<1024>> consensus<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template std::optional<BitCube<1>> cofactor_cube<1>(const BitCube<1>& cube, const BitCube<1>& cofactor) noexcept;
template std::optional<BitCube<2>> cofactor_cube<2>(const BitCube<2>& cube, const BitCube<2>& cofactor) noexcept;
template std::optional<BitCube<4>> cofactor_cube<4>(const BitCube<4>& cube, const BitCube<4>& cofactor) noexcept;
template std::optional<BitCube<8>> cofactor_cube<8>(const BitCube<8>& cube, const BitCube<8>& cofactor) noexcept;
template std::optional<BitCube<16>> cofactor_cube<16>(const BitCube<16>& cube, const BitCube<16>& cofactor) noexcept;
template std::optional<BitCube<32>> cofactor_cube<32>(const BitCube<32>& cube, const BitCube<32>& cofactor_cube) noexcept;
template std::optional<BitCube<64>> cofactor_cube<64>(const BitCube<64>& cube, const BitCube<64>& cofactor_cube) noexcept;
template std::optional<BitCube<128>> cofactor_cube<128>(const BitCube<128>& cube, const BitCube<128>& cofactor_cube) noexcept;
template std::optional<BitCube<256>> cofactor_cube<256>(const BitCube<256>& cube, const BitCube<256>& cofactor_cube) noexcept;
template std::optional<BitCube<512>> cofactor_cube<512>(const BitCube<512>& cube, const BitCube<512>& cofactor_cube) noexcept;
template std::optional<BitCube<1024>> cofactor_cube<1024>(const BitCube<1024>& cube, const BitCube<1024>& cofactor_cube) noexcept;

template Cover<1> cofactor_cover<1>(const Cover<1>& cover, const BitCube<1>& cofactor) noexcept;
template Cover<2> cofactor_cover<2>(const Cover<2>& cover, const BitCube<2>& cofactor) noexcept;
template Cover<4> cofactor_cover<4>(const Cover<4>& cover, const BitCube<4>& cofactor) noexcept;
template Cover<8> cofactor_cover<8>(const Cover<8>& cover, const BitCube<8>& cofactor) noexcept;
template Cover<16> cofactor_cover<16>(const Cover<16>& cover, const BitCube<16>& cofactor) noexcept;
template Cover<32> cofactor_cover<32>(const Cover<32>& cover, const BitCube<32>& cofactor_cube) noexcept;
template Cover<64> cofactor_cover<64>(const Cover<64>& cover, const BitCube<64>& cofactor_cube) noexcept;
template Cover<128> cofactor_cover<128>(const Cover<128>& cover, const BitCube<128>& cofactor_cube) noexcept;
template Cover<256> cofactor_cover<256>(const Cover<256>& cover, const BitCube<256>& cofactor_cube) noexcept;
template Cover<512> cofactor_cover<512>(const Cover<512>& cover, const BitCube<512>& cofactor_cube) noexcept;
template Cover<1024> cofactor_cover<1024>(const Cover<1024>& cover, const BitCube<1024>& cofactor_cube) noexcept;

template std::optional<size_t> single_active_variable<1>(const BitCube<1>& cube) noexcept;
template std::optional<size_t> single_active_variable<2>(const BitCube<2>& cube) noexcept;
template std::optional<size_t> single_active_variable<4>(const BitCube<4>& cube) noexcept;
template std::optional<size_t> single_active_variable<8>(const BitCube<8>& cube) noexcept;
template std::optional<size_t> single_active_variable<16>(const BitCube<16>& cube) noexcept;
template std::optional<size_t> single_active_variable<32>(const BitCube<32>& cube) noexcept;
template std::optional<size_t> single_active_variable<64>(const BitCube<64>& cube) noexcept;
template std::optional<size_t> single_active_variable<128>(const BitCube<128>& cube) noexcept;
template std::optional<size_t> single_active_variable<256>(const BitCube<256>& cube) noexcept;
template std::optional<size_t> single_active_variable<512>(const BitCube<512>& cube) noexcept;
template std::optional<size_t> single_active_variable<1024>(const BitCube<1024>& cube) noexcept;

template bool share_active_variables<1>(const BitCube<1>& a, const BitCube<1>& b, const BitCube<1>& mask) noexcept;
template bool share_active_variables<2>(const BitCube<2>& a, const BitCube<2>& b, const BitCube<2>& mask) noexcept;
template bool share_active_variables<4>(const BitCube<4>& a, const BitCube<4>& b, const BitCube<4>& mask) noexcept;
template bool share_active_variables<8>(const BitCube<8>& a, const BitCube<8>& b, const BitCube<8>& mask) noexcept;
template bool share_active_variables<16>(const BitCube<16>& a, const BitCube<16>& b, const BitCube<16>& mask) noexcept;
template bool share_active_variables<32>(const BitCube<32>& a, const BitCube<32>& b, const BitCube<32>& mask) noexcept;
template bool share_active_variables<64>(const BitCube<64>& a, const BitCube<64>& b, const BitCube<64>& mask) noexcept;
template bool share_active_variables<128>(const BitCube<128>& a, const BitCube<128>& b, const BitCube<128>& mask) noexcept;
template bool share_active_variables<256>(const BitCube<256>& a, const BitCube<256>& b, const BitCube<256>& mask) noexcept;
template bool share_active_variables<512>(const BitCube<512>& a, const BitCube<512>& b, const BitCube<512>& mask) noexcept;
template bool share_active_variables<1024>(const BitCube<1024>& a, const BitCube<1024>& b, const BitCube<1024>& mask) noexcept;

template BitCube<1> disjoint_variables<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template BitCube<2> disjoint_variables<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template BitCube<4> disjoint_variables<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template BitCube<8> disjoint_variables<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template BitCube<16> disjoint_variables<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template BitCube<32> disjoint_variables<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template BitCube<64> disjoint_variables<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template BitCube<128> disjoint_variables<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template BitCube<256> disjoint_variables<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template BitCube<512> disjoint_variables<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template BitCube<1024> disjoint_variables<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template bool is_full_row<1>(const BitCube<1>& cube, const BitCube<1>& cofactor) noexcept;
template bool is_full_row<2>(const BitCube<2>& cube, const BitCube<2>& cofactor) noexcept;
template bool is_full_row<4>(const BitCube<4>& cube, const BitCube<4>& cofactor) noexcept;
template bool is_full_row<8>(const BitCube<8>& cube, const BitCube<8>& cofactor) noexcept;
template bool is_full_row<16>(const BitCube<16>& cube, const BitCube<16>& cofactor) noexcept;
template bool is_full_row<32>(const BitCube<32>& cube, const BitCube<32>& cofactor) noexcept;
template bool is_full_row<64>(const BitCube<64>& cube, const BitCube<64>& cofactor) noexcept;
template bool is_full_row<128>(const BitCube<128>& cube, const BitCube<128>& cofactor) noexcept;
template bool is_full_row<256>(const BitCube<256>& cube, const BitCube<256>& cofactor) noexcept;
template bool is_full_row<512>(const BitCube<512>& cube, const BitCube<512>& cofactor) noexcept;
template bool is_full_row<1024>(const BitCube<1024>& cube, const BitCube<1024>& cofactor) noexcept;

template BitCube<1> force_lower<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template BitCube<2> force_lower<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template BitCube<4> force_lower<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template BitCube<8> force_lower<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template BitCube<16> force_lower<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template BitCube<32> force_lower<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template BitCube<64> force_lower<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template BitCube<128> force_lower<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template BitCube<256> force_lower<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template BitCube<512> force_lower<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template BitCube<1024> force_lower<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template bool cube_contains<1>(const BitCube<1>& a, const BitCube<1>& b) noexcept;
template bool cube_contains<2>(const BitCube<2>& a, const BitCube<2>& b) noexcept;
template bool cube_contains<4>(const BitCube<4>& a, const BitCube<4>& b) noexcept;
template bool cube_contains<8>(const BitCube<8>& a, const BitCube<8>& b) noexcept;
template bool cube_contains<16>(const BitCube<16>& a, const BitCube<16>& b) noexcept;
template bool cube_contains<32>(const BitCube<32>& a, const BitCube<32>& b) noexcept;
template bool cube_contains<64>(const BitCube<64>& a, const BitCube<64>& b) noexcept;
template bool cube_contains<128>(const BitCube<128>& a, const BitCube<128>& b) noexcept;
template bool cube_contains<256>(const BitCube<256>& a, const BitCube<256>& b) noexcept;
template bool cube_contains<512>(const BitCube<512>& a, const BitCube<512>& b) noexcept;
template bool cube_contains<1024>(const BitCube<1024>& a, const BitCube<1024>& b) noexcept;

template Cover<1> cube_complement<1>(const BitCube<1>& cube);
template Cover<2> cube_complement<2>(const BitCube<2>& cube);
template Cover<4> cube_complement<4>(const BitCube<4>& cube);
template Cover<8> cube_complement<8>(const BitCube<8>& cube);
template Cover<16> cube_complement<16>(const BitCube<16>& cube);
template Cover<32> cube_complement<32>(const BitCube<32>& cube);
template Cover<64> cube_complement<64>(const BitCube<64>& cube);
template Cover<128> cube_complement<128>(const BitCube<128>& cube);
template Cover<256> cube_complement<256>(const BitCube<256>& cube);
template Cover<512> cube_complement<512>(const BitCube<512>& cube);
template Cover<1024> cube_complement<1024>(const BitCube<1024>& cube);

template Cover<1> cover_union<1>(const Cover<1>& a, const Cover<1>& b);
template Cover<2> cover_union<2>(const Cover<2>& a, const Cover<2>& b);
template Cover<4> cover_union<4>(const Cover<4>& a, const Cover<4>& b);
template Cover<8> cover_union<8>(const Cover<8>& a, const Cover<8>& b);
template Cover<16> cover_union<16>(const Cover<16>& a, const Cover<16>& b);
template Cover<32> cover_union<32>(const Cover<32>& a, const Cover<32>& b);
template Cover<64> cover_union<64>(const Cover<64>& a, const Cover<64>& b);
template Cover<128> cover_union<128>(const Cover<128>& a, const Cover<128>& b);
template Cover<256> cover_union<256>(const Cover<256>& a, const Cover<256>& b);
template Cover<512> cover_union<512>(const Cover<512>& a, const Cover<512>& b);
template Cover<1024> cover_union<1024>(const Cover<1024>& a, const Cover<1024>& b);

template bool cover_contains<1>(const Cover<1>& a, const Cover<1>& b) noexcept;
template bool cover_contains<2>(const Cover<2>& a, const Cover<2>& b) noexcept;
template bool cover_contains<4>(const Cover<4>& a, const Cover<4>& b) noexcept;
template bool cover_contains<8>(const Cover<8>& a, const Cover<8>& b) noexcept;
template bool cover_contains<16>(const Cover<16>& a, const Cover<16>& b) noexcept;
template bool cover_contains<32>(const Cover<32>& a, const Cover<32>& b) noexcept;
template bool cover_contains<64>(const Cover<64>& a, const Cover<64>& b) noexcept;
template bool cover_contains<128>(const Cover<128>& a, const Cover<128>& b) noexcept;
template bool cover_contains<256>(const Cover<256>& a, const Cover<256>& b) noexcept;
template bool cover_contains<512>(const Cover<512>& a, const Cover<512>& b) noexcept;
template bool cover_contains<1024>(const Cover<1024>& a, const Cover<1024>& b) noexcept;

template Cover<1> remove_duplicates<1>(const Cover<1>& cover);
template Cover<2> remove_duplicates<2>(const Cover<2>& cover);
template Cover<4> remove_duplicates<4>(const Cover<4>& cover);
template Cover<8> remove_duplicates<8>(const Cover<8>& cover);
template Cover<16> remove_duplicates<16>(const Cover<16>& cover);
template Cover<32> remove_duplicates<32>(const Cover<32>& cover);
template Cover<64> remove_duplicates<64>(const Cover<64>& cover);
template Cover<128> remove_duplicates<128>(const Cover<128>& cover);
template Cover<256> remove_duplicates<256>(const Cover<256>& cover);
template Cover<512> remove_duplicates<512>(const Cover<512>& cover);
template Cover<1024> remove_duplicates<1024>(const Cover<1024>& cover);

template Cover<1> remove_contained<1>(const Cover<1>& cover);
template Cover<2> remove_contained<2>(const Cover<2>& cover);
template Cover<4> remove_contained<4>(const Cover<4>& cover);
template Cover<8> remove_contained<8>(const Cover<8>& cover);
template Cover<16> remove_contained<16>(const Cover<16>& cover);
template Cover<32> remove_contained<32>(const Cover<32>& cover);
template Cover<64> remove_contained<64>(const Cover<64>& cover);
template Cover<128> remove_contained<128>(const Cover<128>& cover);
template Cover<256> remove_contained<256>(const Cover<256>& cover);
template Cover<512> remove_contained<512>(const Cover<512>& cover);
template Cover<1024> remove_contained<1024>(const Cover<1024>& cover);
} // namespace espresso::cube_algebra
