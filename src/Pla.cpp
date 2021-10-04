//
// Created by yqszxx on 8/29/21.
//

#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "Pla.hpp"
#include "Cube.hpp"

using namespace espresso;

PLA::PLA() {
    parse();
}

void PLA::parse() {
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
//        std::cerr << "Reached EOF" << std::endl;
    }
}

void PLA::dump() const {
    std::cout << ".i " << Cube::getInputCount() << std::endl;
    std::cout << ".o " << Cube::getOutputCount() << std::endl;
    std::cout << ".type f" << std::endl;
    std::cout <<
        F.toString('1', '0');
}

void elimLowering(Cover& bb, Cover& cc, const Cube& raise, const Cube& freeSet) {
    Cube r = raise + freeSet;

    for (auto& c: bb.cubes) {
        if (!c->isActive) continue;
        if (c->cdist(r) != 0) {
            bb.activeCount--;
            c->isActive = false;
        }
    }

    for (auto& c: bb.cubes) {
        if (!c->isActive) continue;
        if (!c->contains(r)) {
            cc.activeCount--;
            c->isActive = false;
        }
    }
}

void essentialParts(Cover& bb, Cover& cc, const Cube& raise, Cube& freeSet) {
    Cube xLower;

    for (auto& c: bb.cubes) {
        if (!c->isActive) continue;
        unsigned int dist = c->cdist(raise);
        if (dist == 0) {
            assert(0);
        } else if (dist == 1) {
            c->forceLower(xLower, raise);
            bb.activeCount--;
            c->isActive = false;
        }
    }

    if (!xLower.isEmpty()) {
        freeSet -= xLower;
        elimLowering(bb, cc, raise, freeSet);
    }
}

void essenRaising(Cover& bb, Cube& raise, Cube& freeSet) {
    Cube xraise;

    for (auto& c: bb.cubes) {
        if (c->isActive) xraise += *c;
    }

    xraise = freeSet - xraise;

    raise += xraise;
    freeSet -= xraise;
}

bool feasiblyCovered(Cover& bb, Cube& c, Cube& raise, Cube& newLower) {
    Cube r = raise + c;

    for (auto& b: bb.cubes) {
        if (!b->isActive) continue;
        unsigned int dist = b->cdist(r);
        if (dist == 0) {
            return false;
        } else if (dist == 1) {
            b->forceLower(newLower, r);
        }
    }
    return true;
}

unsigned int selectFeasible(Cover& bb, Cover& cc, Cube& raise, Cube& freeSet, Cube& superCube) {
    unsigned int numCovered = 0;
    std::vector<std::shared_ptr<Cube>> feas;
    for (auto& c: cc.cubes) {
        if (c->isActive) feas.push_back(c);
    }
    std::vector<Cube> feasNewLower;
    feasNewLower.resize(cc.activeCount);
    int bestFeas = 0;

    unsigned int numfeas = feas.size(), lastfeas;
    while (true) {
        essenRaising(bb, raise, freeSet);
        lastfeas = numfeas;
        numfeas = 0;
        for (auto i = 0; i < lastfeas; i++) {
            auto& p = feas[i];
            if (p->isActive) {
                if (*p <= raise) {
                    numCovered++;
                    superCube += *p;
                    cc.activeCount--;
                    p->isActive = false;
                    p->isCovered = true;
                } else if (feasiblyCovered(bb, *p, raise, feasNewLower[numfeas])) {
                    feas[numfeas] = p;
                    numfeas++;
                }
            }
        }
        if (numfeas == 0) {  // feasibly_covered
            break;
        }
        unsigned int  bestCount = 0;
        unsigned int bestSize = 9999;
        for (auto i = 0; i < numfeas; i++) {
            unsigned int size = feas[i]->dist(freeSet);
            unsigned int count = 0;

            for (auto j = 0; j < numfeas; j ++) {
                if (feasNewLower[i] <=> *feas[j]) {
                    count++;
                }
            }

            if (count > bestCount) {
                bestCount = count;
                bestFeas = i;
                bestSize = size;
            } else if (count == bestCount && size < bestSize) {
                bestFeas = i;
                bestSize = size;
            }
        }

        raise += *feas[bestFeas];
        freeSet -= raise;
        essentialParts(bb, cc, raise, freeSet);
    }

    return numCovered;
}

unsigned int mostFrequent(Cover& cc, Cube& freeSet) {
    std::vector<unsigned int> count(Cube::getCount(), 0);
    for (auto& c: cc.cubes) {
        if (!c->isActive) continue;
        for (auto i = 0; i < Cube::getCount(); i++) {
            if (c->test(i)) {
                count[i]++;
            }
        }
    }
    unsigned int bestCount = 0;
    unsigned int bestPart = 0;
    for (auto i = 0; i < Cube::getCount(); i++) {
        if (freeSet.test(i) && count[i] > bestCount) {
            bestPart = i;
            bestCount = count[i];
        }
    }
    return bestPart;
}

unsigned int mostFrequent(Cube& freeSet) {
    std::vector<unsigned int> count(Cube::getCount(), 0);

    unsigned int bestCount = 0;
    unsigned int bestPart = 0;
    for (auto i = 0; i < Cube::getCount(); i++) {
        if (freeSet.test(i) && count[i] > bestCount) {
            bestPart = i;
            bestCount = count[i];
        }
    }
    return bestPart;
}

Cover unravelOutput(Cover& b) {
    Cover b1;

    for (auto& c: b.cubes) {
        Cube input = *c & Cube::getInputMask();
        for (auto i = Cube::getInputCount() * 2; i < Cube::getCount(); i++) {
            if (c->test(i)) {
                std::shared_ptr<Cube> c1 = std::make_shared<Cube>(*c);
                c1->set(i);
                b1.insert(c1);
            }
        }
    }

    return b1;
}

void mincov(Cover& bb, Cube& raise, Cube& freeSet) {
    Cover b;
    for (auto& c: bb.cubes) {
        if (!c->isActive) continue;
        std::shared_ptr<Cube> newCube = std::make_shared<Cube>();
        c->forceLower(*newCube, raise);
        b.insert(newCube);
    }

    unsigned int nset = 0;
    bool heuristic = false;
    for (auto& c: b.cubes) {
        unsigned int expansion = 1;
        unsigned int dist = c->dist(Cube::getOutputMask());
        if (dist > 1) {
            expansion *= dist;
            if (expansion > 500) {
                heuristic = true;
                break;
            }
        }
        nset += expansion;
        if (nset > 500) {
            heuristic = true;
            break;
        }
    }

    if (heuristic) {
        raise.set(mostFrequent(freeSet));
        freeSet -= raise;
        essentialParts(bb, *(new Cover), raise, freeSet);
    } else {
        Cover bbUnraveled = unravelOutput(bb);
        Cube SMMincov(Cover& A); // todo: replace this with custom design
        Cube xlower = SMMincov(bbUnraveled);
        Cube xraise = freeSet - xlower;
        bb.activeCount = 0;
        freeSet.reset();
    }
}

void PLA::expand() {
    F.miniSort(true);

    Cube initLower(Cube::getOutputMask());

    for (auto& c: F.cubes) {
        c->isCovered = false;
        c->isNonessential = false;
    }

    for (auto& c: F.cubes) {
        if (!c->isPrime && !c->isCovered) {
            c->isPrime = true;

            R.activeCount = R.cubes.size();
            for (auto& r: R.cubes) {
                r->isActive = true;
            }

            F.activeCount = F.cubes.size();
            for (auto& ff: F.cubes) {
                if (ff->isCovered || ff->isPrime) {
                    F.activeCount--;
                    ff->isActive = false;
                } else {
                    ff->isActive = true;
                }
            }

            unsigned int numCovered = 0;
            Cube superCube = *c;
            Cube raise = *c;
            Cube freeSet = Cube::getFullSet() - raise;

            if (!initLower.isEmpty()) {
                freeSet -= initLower;
                elimLowering(R, F, raise, freeSet);
            }

            essentialParts(R, F, raise, freeSet);

            Cube overexpandedCube = raise + freeSet;

            if (F.activeCount > 0) {
                numCovered += selectFeasible(R, F, raise, freeSet, superCube);
            }

            while (F.activeCount > 0) {
                unsigned int bestIndex = mostFrequent(F, freeSet);
                raise.set(bestIndex);
                freeSet.reset(bestIndex);
                essentialParts(R, F, raise, freeSet);
            }

            while (R.activeCount > 0) {
                mincov(R, raise, freeSet);
            }

            raise += freeSet;
            c->isPrime = true;
            c->isCovered = false;

            if (numCovered == 0 && *c != overexpandedCube) {
                c->isNonessential = true;
            }
        }
    }
}
