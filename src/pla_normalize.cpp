/**
 * @file pla_normalize.cpp
 * @brief PLA file normalization utility
 *
 * This program reads a PLA file, normalizes it (sorts cubes, standardizes format),
 * and writes the normalized result. only supports and pipes.
 *
 * Usage:
 *   cat input.pla | pla_normalize     # reads from stdin, outputs to stdout
 *   cat input.pla | pla_normalize - output.pla  # reads from stdin, writes to file
 */

#include <espresso/pla_parser.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <exception>

using namespace espresso;


template<size_t WIDTH>
bool try_normalize_pla(std::istream& input, std::ostream& output) {
    try {
        // Parse the PLA file
        auto data = PLAParser<WIDTH>::parse(input);
        auto& cover = data.on_set;
        size_t input_count = data.num_inputs;
        size_t output_count = data.num_outputs;
        
        // Convert cover to vector for sorting
        std::vector<typename PLAParser<WIDTH>::CubeType> cubes;
        cubes.reserve(cover.size());
        
        for (const auto& cube : cover) {
            // Check if cube is empty (all bits are zero)
            if (cube.count() == 0) {
                throw PLAParseError("Empty cube detected - cube has no set bits");
            }
            cubes.push_back(cube);
        }
        
        // Sort cubes lexicographically for consistent output
        std::sort(cubes.begin(), cubes.end(), [](const auto& a, const auto& b) {
            // Compare cubes bit by bit for lexicographic ordering
            for (size_t i = 0; i < WIDTH; ++i) {
                bool a_bit = a.test(i);
                bool b_bit = b.test(i);
                if (a_bit != b_bit) {
                    return a_bit < b_bit;  // false < true
                }
            }
            return false;  // Equal cubes
        });
        
        // Remove duplicate cubes while preserving order
        auto last = std::unique(cubes.begin(), cubes.end());
        cubes.erase(last, cubes.end());
        
        // Create a new cover with sorted, deduplicated cubes
        Cover<WIDTH> normalized_cover;
        for (const auto& cube : cubes) {
            normalized_cover.add(cube);
        }
        
        // Write normalized PLA
        PLAParser<WIDTH>::write_pla(output, normalized_cover, input_count, output_count);
        
        return true;
        
    } catch (const PLAParseError& e) {
        // This width didn't work, try next
        return false;
    }
}

int main(int argc, char* argv[]) {
    try {
        // Just use the largest bit width (1024) to handle all cases
        bool success = try_normalize_pla<1024>(std::cin, std::cout);
        
        if (!success) {
            std::cerr << "Error: Failed to parse PLA file" << std::endl;
            return 1;
        }
        std::cout.flush();
        return 0;
        
    } catch (const PLAParseError& e) {
        std::cerr << "PLA Parse Error: " << e.what() << std::endl;
        if (e.line_number() > 0) {
            std::cerr << "At line: " << e.line_number() << std::endl;
        }
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
