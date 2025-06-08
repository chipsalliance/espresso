/**
 * @file espresso.cpp
 * @brief Demonstration program for the modern C++23 PLA parser integrated with Espresso algorithm.
 *
 * This program shows how to:
 * 1. Parse a PLA file using the modern parser
 * 2. Run the Espresso minimization algorithm on the parsed data
 * 3. Write the minimized result back to a PLA file
 */

#include <espresso/pla_parser.hpp>
#include <espresso/espresso_algorithm.hpp>
#include <iostream>
#include <string>
#include <exception>

using namespace espresso;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.pla> [output.pla|roundtrip]\n";
        std::cerr << "Reads a PLA file, optionally minimizes it using Espresso, and writes the result.\n";
        std::cerr << "If 'roundtrip' is specified as second argument, tests round-trip parsing.\n";
        return 1;
    }
    
    const std::string input_filename = argv[1];
    const std::string mode = argc > 2 ? argv[2] : "test";
    const bool roundtrip_mode = (mode == "roundtrip");
    const bool test_mode = (argc == 2 || mode == "test");
    
    try {
        if (!test_mode) {
            std::cout << "Reading PLA file: " << input_filename << std::endl;
        }
        
        // Try to determine appropriate bit width based on input count
        // Start with 64-bit and escalate as needed
        bool success = false;
        
        // Helper lambda to try parsing with a specific width
        auto try_parse_width = [&](auto width_tag) -> bool {
            constexpr uint64_t WIDTH = decltype(width_tag)::value;
            try {
                PLAParser<WIDTH> parser;
                auto [cover, input_count, output_count] = parser.parse_file_tuple(input_filename);
                
                if (!test_mode) {
                    std::cout << "Successfully parsed PLA file with " << WIDTH << "-bit width:" << std::endl;
                    std::cout << "  Inputs: " << input_count << std::endl;
                    std::cout << "  Outputs: " << output_count << std::endl;
                    std::cout << "  Cubes: " << cover.size() << std::endl;
                }
                
                if (roundtrip_mode) {
                    // Test round-trip: write and re-parse
                    std::string temp_file = input_filename + ".roundtrip_temp";
                    std::ofstream temp_out(temp_file);
                    parser.write_pla(temp_out, cover, input_count, output_count);
                    temp_out.close();
                    
                    auto [new_cover, new_input_count, new_output_count] = parser.parse_file_tuple(temp_file);
                    
                    if (input_count != new_input_count || output_count != new_output_count || 
                        cover.size() != new_cover.size()) {
                        std::cerr << "Round-trip test failed: data mismatch" << std::endl;
                        std::remove(temp_file.c_str());
                        return false;
                    }
                    
                    std::remove(temp_file.c_str());
                    if (!test_mode) {
                        std::cout << "Round-trip test passed!" << std::endl;
                    }
                } else if (!test_mode && argc > 2 && mode != "test") {
                    // Run Espresso minimization and write output
                    const std::string output_filename = mode;
                    
                    EspressoOptions options;
                    options.trace = false;  // Disable tracing for cleaner output
                    options.debug = false;
                    options.max_iterations = 100;
                    
                    EspressoContext<WIDTH> context(options);
                    
                    std::cout << "\nRunning Espresso minimization..." << std::endl;
                    auto minimized_cover = espresso::algorithm::espresso(context, cover, Cover<WIDTH>{}, Cover<WIDTH>{});
                    
                    std::cout << "Minimization complete:" << std::endl;
                    std::cout << "  Original cubes: " << cover.size() << std::endl;
                    std::cout << "  Minimized cubes: " << minimized_cover.size() << std::endl;
                    
                    std::cout << "\nWriting minimized PLA to: " << output_filename << std::endl;
                    std::ofstream output_file(output_filename);
                    if (!output_file) {
                        std::cerr << "Error: Could not open output file: " << output_filename << std::endl;
                        return false;
                    }
                    
                    parser.write_pla(output_file, minimized_cover, input_count, output_count);
                    output_file.close();
                    
                    std::cout << "Minimized PLA written successfully." << std::endl;
                }
                
                return true;
            } catch (const PLAParseError& e) {
                if (WIDTH == 1024) {  // Only report error on the largest width attempt
                    throw;
                }
                return false;
            }
        };
        
        // Try different bit widths, starting from smallest to largest
        success = try_parse_width(std::integral_constant<uint64_t, 256>{}) ||
                    try_parse_width(std::integral_constant<uint64_t, 512>{}) ||
                    try_parse_width(std::integral_constant<uint64_t, 1024>{});
        
        if (!success) {
            std::cerr << "Failed to parse PLA file with any supported bit width" << std::endl;
            return 1;
        }
        
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
    }
}
