#include <espresso/bitcube.hpp>
#include <ostream>
#include <stdexcept>
#include <iostream>

namespace espresso {

// BitCube implementations
template <size_t WIDTH>
BitCube<WIDTH>::BitCube() : bits_{} {}

template <size_t WIDTH>
BitCube<WIDTH>::BitCube(std::initializer_list<size_t> ones) : bits_{} {
    for (auto idx : ones) bits_.set(idx);
}

template <size_t WIDTH>
bool BitCube<WIDTH>::operator==(const BitCube& other) const {
    return bits_ == other.bits_;
}

template <size_t WIDTH>
bool BitCube<WIDTH>::operator!=(const BitCube& other) const {
    return !(*this == other);
}

template <size_t WIDTH>
bool BitCube<WIDTH>::test(size_t i) const {
    return bits_.test(i);
}

template <size_t WIDTH>
void BitCube<WIDTH>::set(size_t i, bool v) {
    bits_.set(i, v);
}

template <size_t WIDTH>
void BitCube<WIDTH>::reset() {
    bits_.reset();
}

template <size_t WIDTH>
size_t BitCube<WIDTH>::count() const {
    return bits_.count();
}

template <size_t WIDTH>
size_t BitCube<WIDTH>::size() const {
    return bits_.size();
}

template <size_t WIDTH>
std::string BitCube<WIDTH>::to_string() const {
    std::string s;
    s.reserve(size());
    for (size_t i = 0; i < size(); ++i) s.push_back(bits_.test(i) ? '1' : '0');
    return s;
}

template <size_t WIDTH>
BitCube<WIDTH> BitCube<WIDTH>::from_string(const std::string& s) {
    if (s.size() > WIDTH)
        throw std::invalid_argument("BitCube::from_string: string too long");
    BitCube<WIDTH> c;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '1') c.set(i, true);
        else if (s[i] == '0') c.set(i, false);
        else throw std::invalid_argument("BitCube::from_string: invalid character");
    }
    return c;
}

// Cover implementations
template <size_t WIDTH>
Cover<WIDTH>::Cover() = default;

template <size_t WIDTH>
Cover<WIDTH>::Cover(std::initializer_list<BitCube<WIDTH>> cubes) : cubes_(cubes) {}

template <size_t WIDTH>
void Cover<WIDTH>::add(const BitCube<WIDTH>& c) {
    cubes_.push_back(c);
}

template <size_t WIDTH>
size_t Cover<WIDTH>::size() const {
    return cubes_.size();
}

template <size_t WIDTH>
bool Cover<WIDTH>::empty() const {
    return cubes_.empty();
}

template <size_t WIDTH>
const BitCube<WIDTH>& Cover<WIDTH>::operator[](size_t i) const {
    return cubes_[i];
}

template <size_t WIDTH>
BitCube<WIDTH>& Cover<WIDTH>::operator[](size_t i) {
    return cubes_[i];
}

template <size_t WIDTH>
auto Cover<WIDTH>::begin() -> typename std::vector<BitCube<WIDTH>>::iterator {
    return cubes_.begin();
}

template <size_t WIDTH>
auto Cover<WIDTH>::end() -> typename std::vector<BitCube<WIDTH>>::iterator {
    return cubes_.end();
}

template <size_t WIDTH>
auto Cover<WIDTH>::begin() const -> typename std::vector<BitCube<WIDTH>>::const_iterator {
    return cubes_.begin();
}

template <size_t WIDTH>
auto Cover<WIDTH>::end() const -> typename std::vector<BitCube<WIDTH>>::const_iterator {
    return cubes_.end();
}

template <size_t WIDTH>
bool Cover<WIDTH>::operator==(const Cover<WIDTH>& other) const {
    return cubes_ == other.cubes_;
}

template <size_t WIDTH>
std::vector<std::string> Cover<WIDTH>::to_strings() const {
    std::vector<std::string> v;
    v.reserve(size());
    for (const auto& cube : cubes_) v.push_back(cube.to_string());
    return v;
}

template <size_t WIDTH>
Cover<WIDTH> Cover<WIDTH>::from_strings(const std::vector<std::string>& v) {
    Cover<WIDTH> cover;
    for (const auto& s : v) cover.add(BitCube<WIDTH>::from_string(s));
    return cover;
}

template <size_t WIDTH>
void Cover<WIDTH>::clear() {
    cubes_.clear();
}

} // namespace espresso

// Explicit template instantiations for commonly used sizes
template class espresso::BitCube<1>;
template class espresso::BitCube<2>;
template class espresso::BitCube<4>;
template class espresso::BitCube<8>;
template class espresso::BitCube<16>;
template class espresso::BitCube<32>;
template class espresso::BitCube<64>;
template class espresso::BitCube<128>;
template class espresso::BitCube<256>;
template class espresso::BitCube<512>;
template class espresso::BitCube<1024>;

template class espresso::Cover<1>;
template class espresso::Cover<2>;
template class espresso::Cover<4>;
template class espresso::Cover<8>;
template class espresso::Cover<16>;
template class espresso::Cover<32>;
template class espresso::Cover<64>;
template class espresso::Cover<128>;
template class espresso::Cover<256>;
template class espresso::Cover<512>;
template class espresso::Cover<1024>;
