//
// Created by yqszxx on 8/29/21.
//

#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "Pla.hpp"
#include "Cube.hpp"

espresso::PLA::PLA() {
    parse();
}

void espresso::PLA::parse() {
    std::string line;
    enum {
        FD,
        FR,
    } plaType = FD;
    size_t lineNumber = 1;
    bool optionDone = false;

    // parse from stdin line by line
    while (getline(std::cin, line)) {
        boost::algorithm::trim(line);

        if (line.empty()) continue;

        if (!optionDone && line[0] == '.') {
            line.erase(0, 1);   // remove dot

            if (boost::algorithm::starts_with(line, "i")) {
                line.erase(0, 1);   // remove "i"

                try {
                    Cube::setInputCount(std::stoi(line));
                } catch (std::invalid_argument &e) {
                    boost::algorithm::trim(line);
                    std::cerr << "Insane value for .i: " << line << std::endl;
                    exit(-1);
                } catch (std::logic_error &e) {
                    std::cerr << "Redefinition of .i in line " << lineNumber << std::endl;
                    exit(-1);
                }
            } else if (boost::algorithm::starts_with(line, "o")) {
                line.erase(0, 1);   // remove "o"

                try {
                    Cube::setOutputCount(std::stoi(line));
                } catch (std::invalid_argument &e) {
                    boost::algorithm::trim(line);
                    std::cerr << "Insane value for .o: " << line << std::endl;
                    exit(-1);
                } catch (std::logic_error &e) {
                    std::cerr << "Redefinition of .o in line " << lineNumber << std::endl;
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
            } else {
                std::cerr << "Unknown option: ." << line << std::endl;
                exit(-1);
            }
        } else {
            optionDone = true;

            auto [f, d, r] = Cube::parse(line);

            if (f != nullptr) {
                F.insert(f);
            }
            if (d != nullptr && plaType == FD) {
                D.insert(d);
            }
            if (r != nullptr && plaType == FR) {
                R.insert(r);
            }
        }

        lineNumber++;
    }

    if (std::cin.bad()) {
        std::cerr << "Cannot read from stdin" << std::endl;
    } else if (!std::cin.eof()) {
        std::cerr << "Malformed file" << std::endl;
    } else {
        std::cerr << "Reached EOF" << std::endl;
    }
}

void espresso::PLA::dump() {
    std::cout << ".i " << Cube::getInputCount() << std::endl;
    std::cout << ".o " << Cube::getOutputCount() << std::endl;
    std::cout << ".type fdr" << std::endl;
    std::cout <<
        F.toString('1', '~') <<
        D.toString('-', '~') <<
        R.toString('0', '~');
}
