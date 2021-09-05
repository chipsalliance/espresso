//
// Created by yqszxx on 9/3/21.
//

#include "BitPat.hpp"

espresso::BitPat::BitPat(size_t width) : width(width), value(width), mask(width) {
    mask.set();
}

espresso::BitPat::TristateProxy espresso::BitPat::operator[](size_t pos) {
    if (pos >= width) {
        throw std::out_of_range("BitPat index out of range");
    }
    return {*this, pos};
}

std::string espresso::BitPat::toString() const {
    std::string s;
    for (auto i = 0; i < width; i++) {
        switch (get(i)) {
            case ON:
                s += '1';
                break;
            case OFF:
                s += '0';
                break;
            case DC:
                s += '-';
                break;
        }
    }
    return s;
}

void espresso::BitPat::fromString(const std::string& s) {
    assert(s.length() == width);
    for (auto i = 0; i < width; i++) {
        switch (s[i]) {
            case '0':
                set(i, OFF);
                break;
            case '1':
                set(i, ON);
                break;
            case '-':
                set(i, DC);
                break;
            default:
                throw std::invalid_argument("Unknown character in BitPat");
        }
    }
}

void espresso::BitPat::set(size_t pos, espresso::Tristate t) {
    switch (t) {
        case ON:
            mask.set(pos);
            value.set(pos);
            break;
        case OFF:
            mask.set(pos);
            value.reset(pos);
            break;
        case DC:
            mask.reset(pos);
            value.reset(pos);
            break;
    }
}

espresso::Tristate espresso::BitPat::get(size_t pos) const {
    if (!mask.test(pos)) {
        return DC;
    } else if (value.test(pos)) {
        return ON;
    } else {
        return OFF;
    }
}

std::string espresso::BitPat::toString(char on, char off) const {
    std::string s;
    for (auto i = 0; i < width; i++) {
        switch (get(i)) {
            case ON:
                s += on;
                break;
            case OFF:
                s += off;
                break;
            default:
                break;
        }
    }
    return s;
}

espresso::BitPat::TristateProxy::TristateProxy(espresso::BitPat &bp, size_t pos) : bp(bp), pos(pos) {

}

espresso::BitPat::TristateProxy::operator Tristate () const {
    return bp.get(pos);
}

espresso::BitPat::TristateProxy &espresso::BitPat::TristateProxy::operator=(const espresso::Tristate t) {
    bp.set(pos, t);
    return *this;
}
