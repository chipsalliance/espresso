//
// Created by yqszxx on 8/24/21.
//

#include "Cube.hpp"
#include <boost/algorithm/string.hpp>

size_t espresso::Cube::inputCount = 0;
size_t espresso::Cube::outputCount = 0;
std::unique_ptr<espresso::Cube> espresso::Cube::inputMask = nullptr;
std::unique_ptr<espresso::Cube> espresso::Cube::outputMask = nullptr;
std::unique_ptr<espresso::Cube> espresso::Cube::fullSet = nullptr;

espresso::Cube::Cube() {
    if (inputCount == 0 || outputCount == 0) {
        throw std::logic_error("Input count and output count not set up");
    } else {
        data = std::make_unique<Container>(inputCount * 2 + outputCount);
    }
    isActive = false;
    isCovered = false;
    isNonessential = false;
    isPrime = false;
}

espresso::Cube::Cube(const espresso::Cube &old) : Cube() {
    *data = *old.data;
}

std::string espresso::Cube::toString(char on, char off) const {
    std::string s;
    for (auto i = 0; i < inputCount; i++) {
        if (data->test(i * 2) && data->test(i * 2 + 1)) {
            s += '-';
        } else if (data->test(i * 2) && !data->test(i * 2 + 1)) {
            s += '0';
        } else if (!data->test(i * 2) && data->test(i * 2 + 1)) {
            s += '1';
        }
    }
    s += " ";
    for (auto i = 0; i < outputCount; i++) {
        if (data->test(inputCount * 2 + i)) {
            s += on;
        } else {
            s += off;
        }
    }
    return s;
}

void espresso::Cube::setInputCount(size_t i) {
    if (i <= 0) {
        throw std::invalid_argument("input");
    }
    if (inputCount != 0) {
        throw std::logic_error("Reinitialize of cube.inputCount");
    }
    inputCount = i;
}

void espresso::Cube::setOutputCount(size_t o) {
    if (o <= 0) {
        throw std::invalid_argument("output");
    }
    if (outputCount != 0) {
        throw std::logic_error("Reinitialize of cube.outputCount");
    }
    outputCount = o;
}

size_t espresso::Cube::getInputCount() {
    assert(inputCount != 0);
    return inputCount;
}

size_t espresso::Cube::getOutputCount() {
    assert(outputCount != 0);
    return outputCount;
}

std::tuple<
    std::shared_ptr<espresso::Cube>,
    std::shared_ptr<espresso::Cube>,
    std::shared_ptr<espresso::Cube>
> espresso::Cube::parse(std::string s) {
    Cube input;
    std::vector<std::string> io;
    boost::split(io, s, boost::is_any_of(" "));
    auto inputPart = io[0];
    auto outputPart = io[1];
    for (auto i = 0; i < inputCount; i++) {
        switch (inputPart[i]) {
            case '0':
                input.data->set(i * 2);
                input.data->reset(i * 2 + 1);
                break;
            case '1':
                input.data->reset(i * 2);
                input.data->set(i * 2 + 1);
                break;
            case '-':
                input.data->set(i * 2);
                input.data->set(i * 2 + 1);
                break;
            default:
                throw std::invalid_argument("unknown character in input");
        }
    }

    std::shared_ptr<Cube> f = nullptr, d = nullptr, r = nullptr;
    for (auto i = 0; i < outputCount; i++) {
        switch (outputPart[i]) {
            case '0':
                if (r == nullptr) r = std::make_shared<Cube>(input);
                r->data->set(inputCount * 2 + i);
                break;
            case '1':
                if (f == nullptr) f = std::make_shared<Cube>(input);
                f->data->set(inputCount * 2 + i);
                break;
            case '-':
                if (d == nullptr) d = std::make_shared<Cube>(input);
                d->data->set(inputCount * 2 + i);
                break;
            default:
                throw std::invalid_argument("unknown character in output");
        }
    }
    return {f, d, r};
}

size_t espresso::Cube::getCount() {
    return getInputCount() * 2 + getOutputCount();
}

espresso::Cube& espresso::Cube::getOutputMask() {
    if (outputMask == nullptr) {
        outputMask = std::make_unique<Cube>();
        outputMask->data->set(inputCount * 2, outputCount, true);
    }
    return *outputMask;
}

espresso::Cube& espresso::Cube::getFullSet() {
    if (fullSet == nullptr) {
        fullSet = std::make_unique<Cube>();
        fullSet->data->set();
    }
    return *fullSet;
}

bool espresso::Cube::test(size_t pos) const {
    return data->test(pos);
}

espresso::Cube &espresso::Cube::operator=(const espresso::Cube &x) {
    data = std::make_unique<Container>(*(x.data));
    return *this;
}

espresso::Cube espresso::Cube::operator-(const espresso::Cube &x) const {
    Cube r;
    (*r.data) = (*data) & ~(*x.data);
    return r;
}

espresso::Cube &espresso::Cube::operator-=(const espresso::Cube &x) {
    (*data) = (*data) & ~(*x.data);
    return *this;
}

bool espresso::Cube::isEmpty() const {
    return data->none();
}

espresso::Cube espresso::Cube::operator+(const espresso::Cube &x) const {
    Cube r;
    (*r.data) = (*data) | (*x.data);
    return r;
}

unsigned int espresso::Cube::cdist(const espresso::Cube &x) const {
    unsigned int dist = 0;
    for (auto i = 0; i < inputCount; i++) {
        if ((data->test(2 * i) && !data->test(2 * i + 1) && !x.data->test(2 * i) && x.data->test(2 * i + 1)) ||
            (!data->test(2 * i) && data->test(2 * i + 1) && x.data->test(2 * i) && !x.data->test(2 * i + 1)) ||
            (!data->test(2 * i) && !data->test(2 * i + 1)) ||
            (!x.data->test(2 * i) && !x.data->test(2 * i + 1))) {
            dist++;
        }
    }
    for (auto i = 0; i < outputCount; i++) {
        if (data->test(2 * inputCount + i) && x.data->test(2 * inputCount + i)) {
            return dist;
        }
    }
    return dist + 1;
}

bool espresso::Cube::contains(const espresso::Cube &x) const {
    return (*data & ~(*x.data)).none();
}

// todo: eliminate out-arg xLower
void espresso::Cube::forceLower(espresso::Cube &xLower, const espresso::Cube &x) const {
    for (auto i = 0; i < inputCount; i++) {
        if ((data->test(2 * i) && !data->test(2 * i + 1) && !x.data->test(2 * i) && x.data->test(2 * i + 1)) ||
            (!data->test(2 * i) && data->test(2 * i + 1) && x.data->test(2 * i) && !x.data->test(2 * i + 1)) ||
            (!data->test(2 * i) && !data->test(2 * i + 1)) ||
            (!x.data->test(2 * i) && !x.data->test(2 * i + 1))) {
            if (data->test(2 * i)) {  // or-ing
                xLower.data->set(2 * i);
            }
            if (data->test(2 * i + 1)) {  // or-ing
                xLower.data->set(2 * i + 1);
            }
        }
    }
    for (auto i = 0; i < outputCount; i++) {
        if (data->test(2 * inputCount + i) && x.data->test(2 * inputCount + i)) {
            return;
        }
    }
    for (auto i = 0; i < outputCount; i++) {
        if (data->test(2 * inputCount + i)) {  // or-ing
            xLower.data->set(2 * inputCount + i);
        }
    }
}

espresso::Cube &espresso::Cube::operator+=(const espresso::Cube &x) {
    (*data) = (*data) | (*x.data);
    return *this;
}

bool espresso::Cube::operator<=(const espresso::Cube &x) const {
    return ((*data) & ~(*x.data)).none();
}

unsigned int espresso::Cube::dist(const espresso::Cube &x) const {
    return ((*data) & (*x.data)).count();
}

bool espresso::Cube::operator<=>(const espresso::Cube &x) const {
    return ((*data) & (*x.data)).none();
}

void espresso::Cube::set(size_t pos) {
    data->set(pos);
}

void espresso::Cube::reset(size_t pos) {
    data->reset(pos);
}

bool espresso::Cube::operator==(const espresso::Cube &x) const {
    return (*data) == (*x.data);
}

bool espresso::Cube::operator!=(const espresso::Cube &x) const {
    return !((*this) == x);
}

espresso::Cube &espresso::Cube::getInputMask() {
    if (inputMask == nullptr) {
        inputMask = std::make_unique<Cube>();
        inputMask->data->set(0, inputCount * 2, true);
    }
    return *inputMask;
}

espresso::Cube espresso::Cube::operator&(const espresso::Cube &x) const {
    Cube r;
    (*r.data) = (*data) & (*x.data);
    return r;
}

size_t espresso::Cube::count() const {
    return data->count();
}

void espresso::Cube::reset() {
    data->reset();
}
