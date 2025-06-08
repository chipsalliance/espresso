/**
 * @file pla_parser.cpp
 * @brief Implementation of modern C++23 PLA parser and writer.
 */

#include <espresso/pla_parser.hpp>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <ranges>

namespace espresso {

template<size_t WIDTH>
typename PLAParser<WIDTH>::DataType PLAParser<WIDTH>::parse(std::istream& input) {
    ParserState state;
    std::string line;
    
    while (std::getline(input, line)) {
        try {
            parse_line(line, state, input);
        } catch (const PLAParseError& e) {
            // Re-throw with line number if not already set
            if (e.line_number() == 0) {
                throw PLAParseError(e.what(), state.line_number);
            }
            throw;
        }
        state.line_number++;
    }
    
    // Validate final state
    if (!state.inputs_defined) {
        throw PLAParseError("Missing .i directive", state.line_number);
    }
    if (!state.outputs_defined) {
        throw PLAParseError("Missing .o directive", state.line_number);
    }
    
    // Build result
    DataType result;
    result.num_inputs = state.num_inputs;
    result.num_outputs = state.num_outputs;
    result.type = state.type;
    result.on_set = std::move(state.on_set);
    result.dc_set = std::move(state.dc_set);
    result.off_set = std::move(state.off_set);
    
    // Validate and compute missing sets based on type
    result.validate();
    
    // Note: Legacy parser computes OFF-set/DC-set after parsing via complement operations
    // For now, leave the sets as parsed without computing complements
    // This matches the legacy behavior where:
    // - For FD: R = complement(cube2list(F, D)) is computed later
    // - For FR: D = complement(cube1list(d1merge(sf_join(F, R), num_vars-1))) is computed later
    
    return result;
}

template<size_t WIDTH>
typename PLAParser<WIDTH>::DataType PLAParser<WIDTH>::parse_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw PLAParseError("Cannot open file: " + filename);
    }
    
    try {
        return parse(file);
    } catch (const PLAParseError&) {
        throw; // Re-throw with original error info
    } catch (const std::exception& e) {
        throw PLAParseError("I/O error reading file " + filename + ": " + e.what());
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::write(std::ostream& output, const DataType& data, PLAType type) {
    data.validate();
    
    // Write header
    output << ".i " << data.num_inputs << "\n";
    output << ".o " << data.num_outputs << "\n";
    output << ".type " << (type == PLAType::FD ? "fd" : "fr") << "\n";
    
    // Write cubes from ON-set
    for (const auto& cube : data.on_set) {
        write_cube(output, cube, data.num_inputs, data.num_outputs);
    }
    
    // Write cubes from DC-set if type is FD
    if (type == PLAType::FD) {
        for (const auto& cube : data.dc_set) {
            std::string input_part = cube_input_to_string(cube, data.num_inputs);
            std::string output_part(data.num_outputs, '-'); // DC outputs
            output << input_part << " " << output_part << "\n";
        }
    }
    
    // Write end marker
    output << ".e\n";
}

template<size_t WIDTH>
void PLAParser<WIDTH>::write_file(const std::string& filename, const DataType& data, PLAType type) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw PLAParseError("Cannot create output file: " + filename);
    }
    
    try {
        write(file, data, type);
    } catch (const std::exception& e) {
        throw PLAParseError("I/O error writing file " + filename + ": " + e.what());
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::parse_line(const std::string& line, ParserState& state, std::istream& input) {
    auto trimmed = trim(line);
    
    // Skip empty lines and comments
    if (trimmed.empty() || trimmed[0] == '#') {
        return;
    }
    
    // Handle directives (starting with '.' after trimming whitespace)
    if (trimmed[0] == '.') {
        auto [directive, args] = split_directive(trimmed);
        try {
            parse_directive(directive, args, state);
        } catch (const PLAParseError& e) {
            // Legacy parser would print warning and continue for unknown directives
            // But still throw for clearly invalid cases like duplicates
            if (e.what() && std::string(e.what()).find("Unknown directive") != std::string::npos) {
                // Silently ignore unknown directives like legacy parser
                return;
            }
            // Re-throw other directive errors (like duplicates)
            throw;
        }
        return;
    }
    
    // Handle lines that start with whitespace and are not directives - these are directive continuations  
    if (!line.empty() && (line[0] == ' ' || line[0] == '\t') && trimmed[0] != '.') {
        // This is a continuation of a previous directive like .ilb or .ob
        // Legacy parser would handle this - we just skip these lines as they're informational
        return;
    }
    
    // Handle cube data lines
    if (!state.cube_setup_done) {
        if (!state.inputs_defined || !state.outputs_defined) {
            // Skip cube lines if we don't have header info yet (like legacy)
            return;
        }
        setup_cube_encoding(state);
    }
    
    try {
        // Always use the more lenient parse_cube_multiline (legacy behavior)
        parse_cube_multiline(trimmed, state, input);
    } catch (const PLAParseError& e) {
        // Legacy parser would skip bad lines with warning in specific cases
        // Legacy: fprintf(stderr, "(warning): input line #%d ignored\n", lineno); skip_line(fp);
        // Only skip if it's a character-level parsing error, not structural errors
        std::string error_msg = e.what();
        if (error_msg.find("Invalid input character") != std::string::npos ||
            error_msg.find("Invalid output character") != std::string::npos ||
            error_msg.find("Insufficient characters") != std::string::npos) {
            // These are character-level errors that legacy parser would skip
            return;
        }
        // Re-throw structural errors (like format issues)
        throw;
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::parse_directive(std::string_view directive, std::string_view args, ParserState& state) {
    if (directive == ".i") {
        if (state.inputs_defined) {
            throw PLAParseError("Duplicate .i directive");
        }
        
        auto trimmed_args = trim(args);
        if (trimmed_args.empty()) {
            throw PLAParseError("Missing argument for .i directive");
        }
        
        try {
            state.num_inputs = std::stoull(std::string(trimmed_args));
            if (state.num_inputs == 0) {
                throw PLAParseError("Number of inputs must be greater than 0");
            }
            state.inputs_defined = true;
        } catch (const std::exception&) {
            throw PLAParseError("Invalid number in .i directive: " + std::string(trimmed_args));
        }
    }
    else if (directive == ".o") {
        if (state.outputs_defined) {
            throw PLAParseError("Duplicate .o directive");
        }
        
        auto trimmed_args = trim(args);
        if (trimmed_args.empty()) {
            throw PLAParseError("Missing argument for .o directive");
        }
        
        try {
            state.num_outputs = std::stoull(std::string(trimmed_args));
            if (state.num_outputs == 0) {
                throw PLAParseError("Number of outputs must be greater than 0");
            }
            state.outputs_defined = true;
        } catch (const std::exception&) {
            throw PLAParseError("Invalid number in .o directive: " + std::string(trimmed_args));
        }
    }
    else if (directive == ".type") {
        auto trimmed_args = trim(args);
        if (trimmed_args == "fd") {
            state.type = PLAType::FD;
        } else if (trimmed_args == "fr") {
            state.type = PLAType::FR;
        } else {
            throw PLAParseError("Unknown type: " + std::string(trimmed_args) + " (expected 'fd' or 'fr')");
        }
    }
    else if (directive == ".p") {
        // .p directive (number of product terms) is informational only - ignore
    }
    else if (directive == ".ilb") {
        // .ilb directive (input labels) is informational only - ignore for now
    }
    else if (directive == ".ob") {
        // .ob directive (output labels) is informational only - ignore for now
    }
    else if (directive == ".mv") {
        // .mv directive (multi-valued variables) - ignore for now (legacy support)
    }
    else if (directive == ".kiss") {
        // .kiss directive - ignore for now (legacy support)
    }
    else if (directive == ".symbolic") {
        // .symbolic directive - ignore for now (legacy support)  
    }
    else if (directive == ".symbolic-output") {
        // .symbolic-output directive - ignore for now (legacy support)
    }
    else if (directive == ".phase") {
        // .phase directive - ignore for now (legacy support)
    }
    else if (directive == ".pair") {
        // .pair directive - ignore for now (legacy support)
    }
    else if (directive == ".e" || directive == ".end") {
        // End of file marker - could set a flag but not necessary
    }
    else {
        // Unknown directive: print warning and skip like legacy parser
        // Legacy: fprintf(stderr, "%c%s unrecognized\n", ch, word); skip_line(fp);
        throw PLAParseError("Unknown directive: " + std::string(directive), state.line_number);
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::parse_cube_multiline(std::string_view first_line, ParserState& state, std::istream& input) {
    // Collect all characters for this cube, potentially spanning multiple lines
    std::string cube_data;
    cube_data.reserve(state.num_inputs + state.num_outputs + 10); // Extra space for separators
    
    // Add first line
    for (char c : first_line) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            cube_data += c;
        } else if (!cube_data.empty() && cube_data.back() != ' ') {
            cube_data += ' '; // Normalize separators to single space
        }
    }
    
    // Read additional lines if we don't have enough characters yet
    size_t chars_needed = state.num_inputs + state.num_outputs;
    size_t non_space_chars = 0;
    for (char c : cube_data) {
        if (c != ' ' && c != '|' && c != '\t') {
            non_space_chars++;
        }
    }
    
    while (non_space_chars < chars_needed) {
        std::string additional_line;
        if (!std::getline(input, additional_line)) {
            break; // EOF reached
        }
        
        state.line_number++; // Track line numbers for multi-line cubes
        
        // Add characters from additional line
        for (char c : additional_line) {
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                cube_data += c;
                if (c != '|') {
                    non_space_chars++;
                }
            } else if (!cube_data.empty() && cube_data.back() != ' ') {
                cube_data += ' '; // Normalize separators
            }
        }
        
        if (non_space_chars >= chars_needed) {
            break;
        }
    }
    
    // Now parse the collected cube data using direct character extraction
    // Handle case where there might be no explicit separator (legacy behavior)
    
    parse_cube_direct(cube_data, state);
}

template<size_t WIDTH>
void PLAParser<WIDTH>::parse_cube_direct(std::string_view cube_data, ParserState& state) {
    // Remove any spaces/separators from the cube data
    std::string clean_data;
    for (char c : cube_data) {
        if (c != ' ' && c != '|' && c != '\t') {
            clean_data += c;
        }
    }
    
    // Check if we have enough characters
    size_t needed = state.num_inputs + state.num_outputs;
    if (clean_data.length() < needed) {
        throw PLAParseError("Insufficient characters in cube: expected " + 
                          std::to_string(needed) + 
                          ", got " + std::to_string(clean_data.length()));
    }
    
    // Legacy behavior: ignore extra characters (be more tolerant)
    
    // Extract input and output parts directly by position
    // Take only what we need, ignore extra characters (be more tolerant like legacy)
    std::string input_part = clean_data.substr(0, state.num_inputs);
    std::string output_part = clean_data.substr(state.num_inputs, state.num_outputs);
    
    // Create cubes for different output possibilities
    CubeType on_cube, dc_cube, off_cube;
    
    // Parse input part (same for all cube types)
    parse_input_part(input_part, on_cube, state.num_inputs);
    parse_input_part(input_part, dc_cube, state.num_inputs);
    parse_input_part(input_part, off_cube, state.num_inputs);
    
    // Parse output part and determine which sets to add cubes to
    parse_output_part(output_part, on_cube, dc_cube, off_cube, 
                     state.num_inputs, state.num_outputs, state.type);
    
    // Legacy-compatible cube classification
    bool has_on_output = false;
    bool has_dc_output = false; 
    bool has_off_output = false;
    
    for (size_t i = 0; i < state.num_outputs; i++) {
        char output_char = output_part[i];
        if (output_char == '1' || output_char == '4') {
            has_on_output = true;
        } else if (output_char == '2' || output_char == '-') {
            has_dc_output = true;
        } else if (output_char == '3' || output_char == '0') {
            has_off_output = true;
        }
    }
    
    // Add cubes to appropriate sets based on legacy semantics
    if (has_on_output) {
        state.on_set.add(on_cube);
    }
    if (has_dc_output && state.type == PLAType::FD) {
        state.dc_set.add(dc_cube);  
    }
    if (has_off_output && state.type == PLAType::FR) {
        state.off_set.add(off_cube);
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::setup_cube_encoding(ParserState& state) {
    if (!state.inputs_defined || !state.outputs_defined) {
        throw PLAParseError("Cannot parse cubes before .i and .o directives");
    }
    
    // Validate total bit requirements
    size_t total_vars = state.num_inputs + state.num_outputs;
    if (total_vars * 2 > WIDTH) {
        throw PLAParseError("PLA requires " + std::to_string(total_vars * 2) + 
                          " bits but WIDTH is only " + std::to_string(WIDTH));
    }
    
    state.cube_setup_done = true;
}

template<size_t WIDTH>
void PLAParser<WIDTH>::parse_input_part(std::string_view input_str, CubeType& cube, size_t num_inputs) {
    for (size_t i = 0; i < num_inputs; i++) {
        char c = input_str[i];
        size_t bit_pos = i * 2; // Each input variable uses 2 bits
        
        switch (c) {
            case '0':
                // Input = 0: set bit at position 2*i
                cube.set(bit_pos);
                break;
            case '1':
                // Input = 1: set bit at position 2*i+1
                cube.set(bit_pos + 1);
                break;
            case '2':
            case '-':
                // Legacy: '2' or '-' means don't care for inputs
                // Don't care: set both bits (covers both 0 and 1)
                cube.set(bit_pos);
                cube.set(bit_pos + 1);
                break;
            case '?':
            case '~':
                // Don't care: set both bits (covers both 0 and 1)
                cube.set(bit_pos);
                cube.set(bit_pos + 1);
                break;
            default:
                throw PLAParseError("Invalid input character: '" + std::string(1, c) + 
                                  "' (expected '0', '1', '2', '-', '?', or '~')");
        }
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::parse_output_part(std::string_view output_str, CubeType& on_cube, CubeType& dc_cube,
                                        CubeType& off_cube, size_t num_inputs, size_t num_outputs, PLAType type) {
    size_t output_start = num_inputs * 2;
    
    for (size_t i = 0; i < num_outputs; i++) {
        char c = output_str[i];
        size_t bit_pos = output_start + i * 2;
        
        switch (c) {
            case '0':
            case '3':
                // Legacy: '0' or '3' = Output = 0 (OFF-set for FR type)
                on_cube.set(bit_pos);
                dc_cube.set(bit_pos);
                off_cube.set(bit_pos);
                break;
            case '1':
            case '4':
                // Legacy: '1' or '4' = Output = 1 (ON-set)
                on_cube.set(bit_pos + 1);
                dc_cube.set(bit_pos + 1);
                off_cube.set(bit_pos + 1);
                break;
            case '2':
            case '-':
                // Legacy: '2' or '-' = Don't care output (DC-set for FD type)
                on_cube.set(bit_pos);
                on_cube.set(bit_pos + 1);
                dc_cube.set(bit_pos);
                dc_cube.set(bit_pos + 1);
                off_cube.set(bit_pos);
                off_cube.set(bit_pos + 1);
                break;
            case '~':
            case '?':
                // Don't care output
                on_cube.set(bit_pos);
                on_cube.set(bit_pos + 1);
                dc_cube.set(bit_pos);
                dc_cube.set(bit_pos + 1);
                off_cube.set(bit_pos);
                off_cube.set(bit_pos + 1);
                break;
            default:
                throw PLAParseError("Invalid output character: '" + std::string(1, c) + 
                                  "' (expected '0', '1', '2', '3', '4', '-', '~', or '?')");
        }
    }
}

template<size_t WIDTH>
void PLAParser<WIDTH>::write_cube(std::ostream& output, const CubeType& cube, size_t num_inputs, size_t num_outputs) {
    // Write input part
    output << cube_input_to_string(cube, num_inputs);
    output << " ";
    // Write output part  
    output << cube_output_to_string(cube, num_inputs, num_outputs);
    output << "\n";
}

template<size_t WIDTH>
std::string PLAParser<WIDTH>::cube_input_to_string(const CubeType& cube, size_t num_inputs) {
    std::string result;
    result.reserve(num_inputs);
    
    for (size_t i = 0; i < num_inputs; i++) {
        size_t bit_pos = i * 2;
        bool bit0 = cube.test(bit_pos);
        bool bit1 = cube.test(bit_pos + 1);
        
        if (bit0 && bit1) {
            result += '-'; // Don't care
        } else if (bit1) {
            result += '1'; // Input = 1
        } else if (bit0) {
            result += '0'; // Input = 0
        } else {
            result += '?'; // Invalid encoding - shouldn't happen
        }
    }
    
    return result;
}

template<size_t WIDTH>
std::string PLAParser<WIDTH>::cube_output_to_string(const CubeType& cube, size_t num_inputs, size_t num_outputs) {
    std::string result;
    result.reserve(num_outputs);
    
    size_t output_start = num_inputs * 2;
    
    for (size_t i = 0; i < num_outputs; i++) {
        size_t bit_pos = output_start + i * 2;
        bool bit0 = cube.test(bit_pos);
        bool bit1 = cube.test(bit_pos + 1);
        
        if (bit0 && bit1) {
            result += '-'; // Don't care
        } else if (bit1) {
            result += '1'; // Output = 1
        } else if (bit0) {
            result += '0'; // Output = 0
        } else {
            result += '?'; // Invalid encoding - shouldn't happen
        }
    }
    
    return result;
}

template<size_t WIDTH>
std::string_view PLAParser<WIDTH>::trim(std::string_view str) {
    auto start = str.find_first_not_of(" \t\r\n\f");
    if (start == std::string_view::npos) {
        return {};
    }
    
    auto end = str.find_last_not_of(" \t\r\n\f");
    return str.substr(start, end - start + 1);
}

template<size_t WIDTH>
std::pair<std::string_view, std::string_view> PLAParser<WIDTH>::split_directive(std::string_view line) {
    auto space_pos = line.find_first_of(" \t", 1); // Start from position 1 to skip the '.'
    if (space_pos == std::string_view::npos) {
        return {line, {}};
    }
    
    return {line.substr(0, space_pos), line.substr(space_pos)};
}

// Explicit template instantiations for common bit widths
template class PLAParser<64>;
template class PLAParser<128>;
template class PLAParser<256>;
template class PLAParser<512>;
template class PLAParser<1024>;

} // namespace espresso
