/**
 * @file pla_parser.hpp
 * @brief Modern C++23 PLA (Programmable Logic Array) file parser and writer.
 *
 * Provides PLAParser for reading and writing PLA format files for Boolean function minimization.
 * Supports the standard PLA format with .i (inputs), .o (outputs), .type (fd/fr), and cube data.
 *
 * The parser integrates with the modern BitCube/Cover template system and provides
 * comprehensive error handling and validation.
 */
#pragma once

#include <espresso/bitcube.hpp>
#include <espresso/cube_algebra.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <stdexcept>
#include <memory>
#include <concepts>

namespace espresso {

/**
 * @brief Exception thrown when PLA parsing fails.
 */
class PLAParseError : public std::runtime_error {
public:
    explicit PLAParseError(const std::string& message, size_t line_number = 0)
        : std::runtime_error(format_message(message, line_number))
        , line_number_(line_number) {}
    
    [[nodiscard]] size_t line_number() const noexcept { return line_number_; }

private:
    size_t line_number_;
    
    static std::string format_message(const std::string& message, size_t line_number) {
        if (line_number > 0) {
            return "PLA Parse Error at line " + std::to_string(line_number) + ": " + message;
        }
        return "PLA Parse Error: " + message;
    }
};

/**
 * @brief PLA file type specification.
 */
enum class PLAType {
    FD,  ///< Function/Don't-care format (default)
    FR,  ///< Function/Reverse format
};



/**
 * @brief Parsed PLA data structure.
 */
template<size_t WIDTH>
struct PLAData {
    size_t num_inputs = 0;      ///< Number of input variables
    size_t num_outputs = 0;     ///< Number of output variables  
    PLAType type = PLAType::FD; ///< PLA type (FD or FR)
    
    Cover<WIDTH> on_set;        ///< ON-set (F)
    Cover<WIDTH> dc_set;        ///< Don't-care set (D)
    Cover<WIDTH> off_set;       ///< OFF-set (R) - computed for FD type
    
    /**
     * @brief Get total number of variables (inputs + outputs).
     */
    [[nodiscard]] size_t total_variables() const noexcept {
        return num_inputs + num_outputs;
    }
    
    /**
     * @brief Validate that the PLA data is consistent.
     */
    void validate() const {
        if (num_inputs == 0) {
            throw PLAParseError("Number of inputs must be greater than 0");
        }
        if (num_outputs == 0) {
            throw PLAParseError("Number of outputs must be greater than 0");
        }
        if (total_variables() * 2 > WIDTH) {
            throw PLAParseError("PLA requires " + std::to_string(total_variables() * 2) + 
                              " bits but WIDTH is only " + std::to_string(WIDTH));
        }
    }
};

/**
 * @brief Modern C++23 PLA parser and writer.
 * 
 * Provides functionality to read and write PLA format files with proper
 * error handling and integration with the BitCube/Cover template system.
 */
template<size_t WIDTH>
class PLAParser {
public:
    using CubeType = BitCube<WIDTH>;
    using CoverType = Cover<WIDTH>;
    using DataType = PLAData<WIDTH>;
    
    /**
     * @brief Parse a PLA file from an input stream.
     * @param input Input stream containing PLA data
     * @return Parsed PLA data structure
     * @throws PLAParseError on parsing errors
     */
    [[nodiscard]] static DataType parse(std::istream& input);
    
    /**
     * @brief Parse a PLA file from a file path.
     * @param filename Path to the PLA file
     * @return Parsed PLA data structure
     * @throws PLAParseError on parsing errors or file I/O errors
     */
    [[nodiscard]] static DataType parse_file(const std::string& filename);
    
    /**
     * @brief Write PLA data to an output stream.
     * @param output Output stream to write to
     * @param data PLA data to write
     * @param type PLA type to use for output format
     */
    static void write(std::ostream& output, const DataType& data, PLAType type = PLAType::FD);
    
    /**
     * @brief Write PLA data to a file.
     * @param filename Path to the output file
     * @param data PLA data to write  
     * @param type PLA type to use for output format
     * @throws PLAParseError on file I/O errors
     */
    static void write_file(const std::string& filename, const DataType& data, PLAType type = PLAType::FD);
    
    /**
     * @brief Write PLA data to an output stream (convenience method with simplified signature).
     * @param output Output stream to write to
     * @param cover Cover to write (ON-set)
     * @param num_inputs Number of input variables
     * @param num_outputs Number of output variables
     */
    static void write_pla(std::ostream& output, const CoverType& cover, size_t num_inputs, size_t num_outputs) {
        DataType data;
        data.num_inputs = num_inputs;
        data.num_outputs = num_outputs;
        data.on_set = cover;
        write(output, data);
    }
    
    /**
     * @brief Parse a PLA file and return tuple (for backward compatibility).
     * @param filename Path to the PLA file
     * @return Tuple of (cover, input_count, output_count)
     * @throws PLAParseError on parsing errors or file I/O errors
     */
    [[nodiscard]] static std::tuple<CoverType, size_t, size_t> parse_file_tuple(const std::string& filename) {
        auto data = parse_file(filename);
        return std::make_tuple(data.on_set, data.num_inputs, data.num_outputs);
    }

private:
    /**
     * @brief Internal parser state.
     */
    struct ParserState {
        size_t line_number = 1;
        size_t num_inputs = 0;
        size_t num_outputs = 0;
        PLAType type = PLAType::FD;
        bool inputs_defined = false;
        bool outputs_defined = false;
        bool cube_setup_done = false;
        CoverType on_set;
        CoverType dc_set;
        CoverType off_set;
    };
    
    /**
     * @brief Parse a single line from the PLA file.
     */
    static void parse_line(const std::string& line, ParserState& state, std::istream& input);
    
    /**
     * @brief Parse a directive line (starting with '.').
     */
    static void parse_directive(std::string_view directive, std::string_view args, ParserState& state);
    
    /**
     * @brief Parse a cube data line.
     */
    
    /**
     * @brief Parse a cube that may span multiple lines.
     */
    static void parse_cube_multiline(std::string_view first_line, ParserState& state, std::istream& input);
    
    /**
     * @brief Parse cube data directly by character position (no separators).
     */
    static void parse_cube_direct(std::string_view cube_data, ParserState& state);
    
    /**
     * @brief Setup cube encoding after inputs/outputs are defined.
     */
    static void setup_cube_encoding(ParserState& state);
    
    /**
     * @brief Parse input part of a cube.
     */
    static void parse_input_part(std::string_view input_str, CubeType& cube, size_t num_inputs);
    
    /**
     * @brief Parse output part of a cube.
     */
    static void parse_output_part(std::string_view output_str, CubeType& on_cube, CubeType& dc_cube, 
                                 CubeType& off_cube, size_t num_inputs, size_t num_outputs, PLAType type);
    
    /**
     * @brief Write a single cube to output stream.
     */
    static void write_cube(std::ostream& output, const CubeType& cube, size_t num_inputs, size_t num_outputs);
    
    /**
     * @brief Convert cube input encoding to string.
     */
    static std::string cube_input_to_string(const CubeType& cube, size_t num_inputs);
    
    /**
     * @brief Convert cube output encoding to string.
     */
    static std::string cube_output_to_string(const CubeType& cube, size_t num_inputs, size_t num_outputs);
    
    /**
     * @brief Trim whitespace from string.
     */
    static std::string_view trim(std::string_view str);
    
    /**
     * @brief Split string into directive and arguments.
     */
    static std::pair<std::string_view, std::string_view> split_directive(std::string_view line);
};

// Type aliases for common bit widths
using PLAParser64 = PLAParser<64>;
using PLAParser128 = PLAParser<128>;
using PLAParser256 = PLAParser<256>;

using PLAData64 = PLAData<64>;
using PLAData128 = PLAData<128>;
using PLAData256 = PLAData<256>;

} // namespace espresso
