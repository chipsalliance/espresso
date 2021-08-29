//
// Created by yqszxx on 8/29/21.
//

#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "Pla.hpp"

espresso::PLA::PLA() : inputCount(0), outputCount(0) {
    parse();
}

void espresso::PLA::parse() {
    std::string line;
    enum {
        FD,
        FR,
    } plaType = FD;

    // parse from stdin line by line
    while (getline(std::cin, line)) {
        boost::algorithm::trim(line);
        if (line.empty()) continue;
        if (line[0] == '.') {
            line.erase(0, 1);   // remove dot

            if (boost::algorithm::starts_with(line, "i")) {
                line.erase(0, 1);   // remove "i"
                try {
                    inputCount = std::stoi(line);
                    if (inputCount <= 0) {
                        throw std::invalid_argument("input");
                    }
                } catch (std::invalid_argument &e) {
                    boost::algorithm::trim(line);
                    std::cerr << "Insane value for .i: " << line << std::endl;
                    exit(-1);
                }
            } else if (boost::algorithm::starts_with(line, "o")) {
                line.erase(0, 1);   // remove "o"
                try {
                    outputCount = std::stoi(line);
                    if (outputCount <= 0) {
                        throw std::invalid_argument("output");
                    }
                } catch (std::invalid_argument &e) {
                    boost::algorithm::trim(line);
                    std::cerr << "Insane value for .o: " << line << std::endl;
                    exit(-1);
                }
            } else if (boost::algorithm::starts_with(line, "type")) {
                line.erase(0, 4);   // remove "type"
                boost::algorithm::trim(line);
                if (line == "fd") {
                    plaType = FD;
                } else if (line == "fr") {
                    plaType = FR;
                } else {
                    std::cerr << "Unknown type: " << line << std::endl;
                    exit(-1);
                }
            } else if (boost::algorithm::starts_with(line, "end")) {
                // DBG
                std::cout << "i = " << inputCount << ", o = " << outputCount << ", type = " << (plaType == FD ? "fd" : "fr") << std::endl;

                return;
            } else {
                std::cerr << "Unknown option: ." << line << std::endl;
                exit(-1);
            }
        }
    }

    if (std::cin.bad()) {
        std::cerr << "Cannot read from stdin" << std::endl;
    } else if (!std::cin.eof()) {
        std::cerr << "Malformed file" << std::endl;
    } else {
        std::cerr << "Reached EOF" << std::endl;
    }
}
