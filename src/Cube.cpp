//
// Created by yqszxx on 8/24/21.
//

#include "Cube.hpp"
#include <boost/algorithm/string.hpp>

size_t espresso::Cube::inputCount = 0;
size_t espresso::Cube::outputCount = 0;

espresso::Cube::Cube() {
    if (inputCount == 0 || outputCount == 0) {
        throw std::logic_error("Input count and output count not set up");
    } else {
        data = std::make_unique<Container>(inputCount * 2 + outputCount);
    }
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
    return inputCount;
}

size_t espresso::Cube::getOutputCount() {
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

