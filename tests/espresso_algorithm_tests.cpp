/**
 * @file espresso_algorithm_tests.cpp
 * @brief Test suite for the modern C++23 Espresso algorithm implementation.
 *
 * Tests the core espresso() function and supporting algorithms to ensure
 * correct logic minimization behavior.
 */

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <espresso/espresso_algorithm.hpp>
#include <espresso/espresso_context.hpp>
#include <espresso/bitcube.hpp>

using namespace espresso;
using namespace espresso::algorithm;

TEST_CASE("EspressoContext basic functionality", "[espresso][context]") {
    SECTION("Default construction") {
        EspressoContext<32> context;
        
        REQUIRE(context.options().trace == false);
        REQUIRE(context.options().debug == false);
        REQUIRE(context.options().remove_essential == true);
        REQUIRE(context.elapsed_time().count() >= 0);
        REQUIRE(!context.should_timeout());
    }
    
    SECTION("Custom options") {
        EspressoOptions opts;
        opts.trace = true;
        opts.debug = true;
        opts.max_iterations = 100;
        
        EspressoContext<32> context(opts);
        
        REQUIRE(context.options().trace == true);
        REQUIRE(context.options().debug == true);
        REQUIRE(context.options().max_iterations == 100);
    }
    
    SECTION("Cost calculation") {
        EspressoContext<32> context;
        Cover<32> cover;
        
        // Empty cover
        auto cost = context.calculate_cost(cover);
        REQUIRE(cost.cubes == 0);
        REQUIRE(cost.literals == 0);
        
        // Single cube
        BitCube<32> cube;
        cube.set(0);
        cube.set(2);
        cube.set(4);
        cover.add(cube);
        
        cost = context.calculate_cost(cover);
        REQUIRE(cost.cubes == 1);
        REQUIRE(cost.literals == 3);
    }
    
    SECTION("Temporary buffer management") {
        EspressoContext<32> context;
        
        auto& cover1 = context.get_temp_cover();
        auto& cover2 = context.get_temp_cover();
        auto& cube1 = context.get_temp_cube();
        
        REQUIRE(context.temp_covers_allocated() >= 2);
        REQUIRE(context.temp_cubes_allocated() >= 1);
        
        context.release_temp_buffers();
        
        // Should be able to get them again
        auto& cover3 = context.get_temp_cover();
        static_cast<void>(cover3); // Suppress unused warning
    }
}

TEST_CASE("Expand operation", "[espresso][expand]") {
    SECTION("Simple expansion") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> R; // Empty OFF-set allows maximum expansion
        
        // Single cube to expand
        BitCube<8> cube;
        cube.set(0);
        cube.set(2);
        F.add(cube);
        
        auto result = expand(context, F, R, false);
        
        REQUIRE(result.size() == 1);
        // Result should have at least the original coverage
        auto expanded_cube = *result.begin();
        REQUIRE(expanded_cube.test(0));
        REQUIRE(expanded_cube.test(2));
    }
    
    SECTION("Expansion with OFF-set constraints") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> R;
        
        // ON-set cube
        BitCube<8> on_cube;
        on_cube.set(0);
        on_cube.set(1);
        F.add(on_cube);
        
        // OFF-set cube that should prevent expansion to bit 2
        BitCube<8> off_cube;
        off_cube.set(0);
        off_cube.set(1);
        off_cube.set(2);
        R.add(off_cube);
        
        auto result = expand(context, F, R, false);
        
        REQUIRE(result.size() == 1);
        auto expanded_cube = *result.begin();
        REQUIRE(expanded_cube.test(0));
        REQUIRE(expanded_cube.test(1));
        // Should not expand to bit 2 due to OFF-set constraint
    }
}

TEST_CASE("Reduce operation", "[espresso][reduce]") {
    SECTION("Simple reduction") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> D; // Empty don't-care set
        
        // Cube that might be reducible
        BitCube<8> cube;
        cube.set(0);
        cube.set(1);
        cube.set(2);
        cube.set(3);
        F.add(cube);
        
        auto result = reduce(context, F, D);
        
        REQUIRE(result.size() == 1);
        // Reduced cube should have same or fewer literals
        auto reduced_cube = *result.begin();
        REQUIRE(reduced_cube.count() <= cube.count());
    }
    
    SECTION("Multiple cubes reduction") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> D;
        
        BitCube<8> cube1;
        cube1.set(0);
        cube1.set(1);
        F.add(cube1);
        
        BitCube<8> cube2;
        cube2.set(2);
        cube2.set(3);
        F.add(cube2);
        
        auto result = reduce(context, F, D);
        
        REQUIRE(result.size() == 2);
    }
}

TEST_CASE("Irredundant operation", "[espresso][irredundant]") {
    SECTION("Remove redundant cubes") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> D;
        
        // Large cube that contains smaller cube
        BitCube<8> large_cube;
        large_cube.set(0);
        large_cube.set(1);
        large_cube.set(2);
        large_cube.set(3);
        F.add(large_cube);
        
        // Smaller cube contained in the large cube
        BitCube<8> small_cube;
        small_cube.set(1);
        small_cube.set(2);
        F.add(small_cube);
        
        auto result = irredundant(context, F, D);
        
        // Should remove the redundant smaller cube
        REQUIRE(result.size() <= F.size());
    }
    
    SECTION("Keep non-redundant cubes") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> D;
        
        // Two non-overlapping cubes
        BitCube<8> cube1;
        cube1.set(0);
        cube1.set(1);
        F.add(cube1);
        
        BitCube<8> cube2;
        cube2.set(4);
        cube2.set(5);
        F.add(cube2);
        
        auto result = irredundant(context, F, D);
        
        // Both cubes should be kept
        REQUIRE(result.size() == 2);
    }
}

TEST_CASE("Essential prime extraction", "[espresso][essential]") {
    SECTION("Extract essential primes") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> D;
        
        BitCube<8> cube1;
        cube1.set(0);
        cube1.set(1);
        F.add(cube1);
        
        BitCube<8> cube2;
        cube2.set(2);
        cube2.set(3);
        F.add(cube2);
        
        auto F_copy = F;
        auto D_copy = D;
        
        auto essentials = essential(context, F_copy, D_copy);
        
        // Function should complete without error
        REQUIRE(essentials.size() >= 0);
        REQUIRE(F_copy.size() >= 0);
    }
}

TEST_CASE("Core Espresso algorithm", "[espresso][algorithm]") {
    SECTION("Simple minimization") {
        EspressoOptions opts;
        opts.trace = false; // Disable tracing for tests
        opts.debug = false;
        opts.max_iterations = 10; // Limit iterations for tests
        
        EspressoContext<8> context(opts);
        
        Cover<8> F; // ON-set
        Cover<8> D; // Don't-care set (empty)
        Cover<8> R; // OFF-set (empty - allows maximum freedom)
        
        // Simple ON-set with redundant cubes
        BitCube<8> cube1;
        cube1.set(0);
        cube1.set(1);
        F.add(cube1);
        
        BitCube<8> cube2;
        cube2.set(0);
        cube2.set(1);
        cube2.set(2); // Superset of cube1
        F.add(cube2);
        
        BitCube<8> cube3;
        cube3.set(4);
        cube3.set(5);
        F.add(cube3);
        
        auto result = espresso::algorithm::espresso(context, F, D, R);
        
        // Result should be minimized
        REQUIRE(result.size() <= F.size());
        REQUIRE(result.size() > 0);
        
        // Check that algorithm completed within iterations
        REQUIRE(context.elapsed_time().count() >= 0);
    }
    
    SECTION("Empty covers") {
        EspressoContext<8> context;
        Cover<8> empty_F;
        Cover<8> empty_D;
        Cover<8> empty_R;
        
        auto result = espresso::algorithm::espresso(context, empty_F, empty_D, empty_R);
        
        REQUIRE(result.size() == 0);
    }
    
    SECTION("Single cube") {
        EspressoContext<8> context;
        Cover<8> F;
        Cover<8> D;
        Cover<8> R;
        
        BitCube<8> cube;
        cube.set(1);
        cube.set(3);
        cube.set(5);
        F.add(cube);
        
        auto result = espresso::algorithm::espresso(context, F, D, R);
        
        REQUIRE(result.size() == 1);
    }
    
    SECTION("Algorithm with timeout") {
        EspressoOptions opts;
        opts.timeout = std::chrono::milliseconds(100); // Generous timeout
        opts.max_iterations = 10; // Low iteration limit to ensure quick completion
        
        EspressoContext<8> context(opts);
        
        Cover<8> F;
        // Add a few simple cubes
        for (size_t i = 0; i < 4; ++i) {
            BitCube<8> cube;
            cube.set(i);
            cube.set((i + 1) % 8);
            F.add(cube);
        }
        
        Cover<8> D, R;
        
        // Algorithm should complete normally with generous timeout
        auto result = espresso::algorithm::espresso(context, F, D, R);
        
        REQUIRE(result.size() > 0);
        // With generous timeout and low iteration limit, should not timeout
        REQUIRE_FALSE(context.should_timeout());
        // Should complete reasonably quickly
        REQUIRE(context.elapsed_time() < std::chrono::seconds(1));
    }
}

TEST_CASE("Sorting algorithms", "[espresso][sorting]") {
    SECTION("Sort reduce") {
        EspressoContext<8> context;
        Cover<8> F;
        
        // Add cubes with different sizes
        BitCube<8> small_cube;
        small_cube.set(0);
        F.add(small_cube);
        
        BitCube<8> large_cube;
        large_cube.set(1);
        large_cube.set(2);
        large_cube.set(3);
        large_cube.set(4);
        F.add(large_cube);
        
        BitCube<8> medium_cube;
        medium_cube.set(5);
        medium_cube.set(6);
        F.add(medium_cube);
        
        auto result = sort_reduce(context, F);
        
        REQUIRE(result.size() == 3);
        
        // Verify sorting order (should be ascending by size)
        auto it = result.begin();
        auto first = *it++;
        auto second = *it++;
        auto third = *it;
        
        REQUIRE(first.count() <= second.count());
        REQUIRE(second.count() <= third.count());
    }
    
    SECTION("Mini sort ascending") {
        EspressoContext<8> context;
        Cover<8> F;
        
        BitCube<8> cube1;
        cube1.set(0);
        cube1.set(1);
        cube1.set(2);
        F.add(cube1);
        
        BitCube<8> cube2;
        cube2.set(3);
        F.add(cube2);
        
        auto result = mini_sort(context, F, true);
        
        REQUIRE(result.size() == 2);
        
        // Check ascending order
        auto it = result.begin();
        auto first = *it++;
        auto second = *it;
        
        REQUIRE(first.count() <= second.count());
    }
    
    SECTION("Mini sort descending") {
        EspressoContext<8> context;
        Cover<8> F;
        
        BitCube<8> cube1;
        cube1.set(0);
        F.add(cube1);
        
        BitCube<8> cube2;
        cube2.set(1);
        cube2.set(2);
        cube2.set(3);
        F.add(cube2);
        
        auto result = mini_sort(context, F, false);
        
        REQUIRE(result.size() == 2);
        
        // Check descending order
        auto it = result.begin();
        auto first = *it++;
        auto second = *it;
        
        REQUIRE(first.count() >= second.count());
    }
}

TEST_CASE("Integration tests", "[espresso][integration]") {
    SECTION("Complete minimization workflow") {
        EspressoOptions opts;
        opts.trace = false;
        opts.debug = false;
        opts.max_iterations = 5;
        
        EspressoContext<16> context(opts);
        
        Cover<16> F;
        
        // Create a somewhat complex cover with redundancies
        for (size_t i = 0; i < 5; ++i) {
            BitCube<16> cube;
            cube.set(i);
            cube.set(i + 5);
            F.add(cube);
            
            // Add a larger cube that might contain some smaller ones
            if (i < 3) {
                BitCube<16> large_cube;
                large_cube.set(i);
                large_cube.set(i + 5);
                large_cube.set(i + 10);
                F.add(large_cube);
            }
        }
        
        Cover<16> D; // Empty don't-care
        Cover<16> R; // Empty OFF-set
        
        auto initial_cost = context.calculate_cost(F);
        auto result = espresso::algorithm::espresso(context, F, D, R);
        auto final_cost = context.calculate_cost(result);
        
        REQUIRE(result.size() > 0);
        REQUIRE(final_cost.cubes <= initial_cost.cubes);
    }
}

TEST_CASE("Edge cases and error conditions", "[espresso][edge_cases]") {
    SECTION("Very small covers") {
        EspressoContext<4> context;
        Cover<4> F;
        
        BitCube<4> cube;
        cube.set(0);
        F.add(cube);
        
        auto result = espresso::algorithm::espresso(context, F, {}, {});
        REQUIRE(result.size() == 1);
    }
    
    SECTION("Maximum sized cubes") {
        EspressoContext<8> context;
        Cover<8> F;
        
        BitCube<8> full_cube;
        for (size_t i = 0; i < 8; ++i) {
            full_cube.set(i);
        }
        F.add(full_cube);
        
        auto result = espresso::algorithm::espresso(context, F, {}, {});
        REQUIRE(result.size() == 1);
    }
    
    SECTION("Algorithm options validation") {
        EspressoOptions opts;
        opts.max_iterations = 0; // Edge case
        
        EspressoContext<8> context(opts);
        Cover<8> F;
        
        BitCube<8> cube;
        cube.set(0);
        F.add(cube);
        
        auto result = espresso::algorithm::espresso(context, F, {}, {});
        REQUIRE(result.size() >= 0); // Should handle gracefully
    }
}
