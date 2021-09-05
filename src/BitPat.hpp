//
// Created by yqszxx on 9/3/21.
//

#ifndef ESPRESSO_BITPAT_HPP
#define ESPRESSO_BITPAT_HPP

#include <boost/dynamic_bitset.hpp>

namespace espresso {

using Tristate = enum Tristate {
    ON,
    OFF,
    DC,
};

class BitPat {
public:
    class TristateProxy {
    public:
        explicit operator Tristate () const;
        TristateProxy &operator = (Tristate t);
        TristateProxy(espresso::BitPat &bp, size_t pos);

    private:
        BitPat &bp;
        size_t pos;
    };

    explicit BitPat(size_t width);
    TristateProxy operator [] (size_t pos);
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::string toString(char on, char off) const;
    void fromString(const std::string& s);

private:
    using Container = boost::dynamic_bitset<>;
    void set(size_t pos, Tristate t);
    [[nodiscard]] Tristate get(size_t pos) const;
    size_t width;
    /**
     * the literal value, with don't cares being 0
     */
    Container value;
    /**
     * the mask bits, with don't cares being 0 and cares being 1
     */
    Container mask;
};

} // namespace espresso


#endif //ESPRESSO_BITPAT_HPP
