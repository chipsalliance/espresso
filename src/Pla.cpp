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
    size_t lineNumber = 1;

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
                    F = std::make_unique<Cover>(coverSize());
                    D = std::make_unique<Cover>(coverSize());
                    R = std::make_unique<Cover>(coverSize());
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
                return;
            } else {
                std::cerr << "Unknown option: ." << line << std::endl;
                exit(-1);
            }
        } else {
            auto inputs = std::make_shared<Cube>(coverSize());
            for (auto i = 0; i < inputCount; i++) {
                if (line.empty()) {
                    std::cerr << "Bad length of line " << lineNumber << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto c = line[0];
                line.erase(0, 1);   // get and remove first character
                switch (c) {
                    case ' ':
                    case '|':
                    case '\t':
                        i--;
                        break;
                    case '-':  // 2n and (2n + 1)
                        inputs->set(i * 2 + 1);
                    case '0':  // 2n
                        inputs->set(i * 2);
                        break;
                    case '1':  // (2n + 1)
                        inputs->set(i * 2 + 1);
                        break;
                    case '?':
                        break;
                    default:
                        std::cerr << "Unknown character in line " << lineNumber << std::endl;
                        exit(EXIT_FAILURE);
                }
            }

            auto newF = std::make_shared<Cube>(*inputs);
            auto newD = std::make_shared<Cube>(*inputs);
            auto newR = std::make_shared<Cube>(*inputs);
            bool saveNewF = false, saveNewD = false, saveNewR = false;
            for (auto i = 0; i < outputCount; i++) {
                if (line.empty()) {
                    std::cerr << "Bad length of line " << lineNumber << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto c = line[0];
                line.erase(0, 1);   // get and remove first character
                switch (c) {
                    case ' ':
                    case '|':
                    case '\t':
                        i--;
                        break;
                    case '4':
                    case '1':
                        newF->set(inputCount * 2 + i);
                        saveNewF = true;
                        break;
                    case '3':
                    case '0':
                        if (plaType == FR) {
                            newR->set(inputCount * 2 + i);
                            saveNewR = true;
                        }
                        break;
                    case '2':
                    case '-':
                        if (plaType == FD) {
                            newD->set(inputCount * 2 + i);
                            saveNewD = true;
                        }
                    case '~':
                        break;
                    default:
                        std::cerr << "Unknown character in line " << lineNumber << std::endl;
                        exit(EXIT_FAILURE);
                }
            }

            if (saveNewF) {
                F->insert(newF);
            }
            if (saveNewD) {
                D->insert(newD);
            }
            if (saveNewR) {
                R->insert(newR);
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

size_t espresso::PLA::coverSize() const {
    return inputCount * 2 + outputCount;
}

void espresso::PLA::dump() {
    std::cout << ".i " << inputCount << std::endl;
    std::cout << ".o " << outputCount << std::endl;
    std::cout << ".type fdr" << std::endl;
    dumpSet('1');
    dumpSet('0');
    dumpSet('-');
    std::cout << ".e" << std::endl;
}

void espresso::PLA::dumpSet(char c) const {
    auto set = (c == '1' ? F : (c == '0' ? R : D));
    for (const auto& e: *(set->data)) {
        for (auto i = 0; i < inputCount; i++) {
            if (e->test(2 * i) && !e->test(2 * i + 1)) {
                std::cout << "0";
            } else if (!e->test(2 * i) && e->test(2 * i + 1)) {
                std::cout << "1";
            } else if (e->test(2 * i) && e->test(2 * i + 1)) {
                std::cout << "-";
            }
        }
        std::cout << " ";
        for (auto i = 0; i < outputCount; i++) {
            if (e->test(2 * inputCount + i)) {
                std::cout << c;
            } else {
                std::cout << "~";
            }
        }
        std::cout << std::endl;
    }
}
