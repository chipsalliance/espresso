#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <espresso/bitcube.hpp>
#include <sstream>

using namespace espresso;

TEST_CASE("BitCube basic operations", "[bitcube]") {
    BitCube<64> c1{0, 2, 4};
    REQUIRE(c1.test(0));
    REQUIRE(!c1.test(1));
    REQUIRE(c1.test(2));
    REQUIRE(c1.count() == 3);
    c1.set(1);
    REQUIRE(c1.test(1));
    c1.set(2, false);
    REQUIRE(!c1.test(2));
    c1.reset();
    REQUIRE(c1.count() == 0);
}

TEST_CASE("Cover and CubeList", "[cover]") {
    BitCube<64> c1{1, 3};
    BitCube<64> c2{2, 4};
    Cover<64> cover{c1, c2};
    REQUIRE(cover.size() == 2);
    REQUIRE(cover[0] == c1);
    REQUIRE(cover[1] == c2);
    cover.add(BitCube<64>{0});
    REQUIRE(cover.size() == 3);
}

TEST_CASE("BitCube copy and assignment", "[bitcube]") {
    BitCube<64> a{1, 5, 7};
    BitCube<64> b = a;
    REQUIRE(a == b);
    b.set(2);
    REQUIRE(b != a);
    a = b;
    REQUIRE(a == b);
}

TEST_CASE("BitCube all bits", "[bitcube]") {
    BitCube<64> c;
    for (size_t i = 0; i < c.size(); ++i) c.set(i);
    REQUIRE(c.count() == c.size());
    for (size_t i = 0; i < c.size(); ++i) REQUIRE(c.test(i));
}

TEST_CASE("BitCube none set", "[bitcube]") {
    BitCube<64> c;
    REQUIRE(c.count() == 0);
    for (size_t i = 0; i < c.size(); ++i) REQUIRE(!c.test(i));
}

TEST_CASE("BitCube set and reset", "[bitcube]") {
    BitCube<64> c;
    c.set(10);
    REQUIRE(c.test(10));
    c.set(10, false);
    REQUIRE(!c.test(10));
    c.set(63);
    REQUIRE(c.test(63));
    c.reset();
    REQUIRE(c.count() == 0);
}

TEST_CASE("Cover add, access, and equality", "[cover]") {
    Cover<64> cover1;
    BitCube<64> c1{0, 1};
    BitCube<64> c2{2, 3};
    cover1.add(c1);
    cover1.add(c2);
    REQUIRE(cover1.size() == 2);
    REQUIRE(cover1[0] == c1);
    REQUIRE(cover1[1] == c2);

    Cover<64> cover2{c1, c2};
    REQUIRE(cover1 == cover2);
    cover2.add(BitCube<64>{4});
    REQUIRE(cover1 != cover2);
}

TEST_CASE("Cover iteration and modification", "[cover]") {
    BitCube<64> c1{1};
    BitCube<64> c2{2};
    BitCube<64> c3{3};
    Cover<64> cover{c1, c2, c3};
    size_t count = 0;
    for (const auto& cube : cover) {
        REQUIRE(cube == cover[count]);
        ++count;
    }
    REQUIRE(count == 3);
    // Modify a cube
    cover[1].set(5);
    REQUIRE(cover[1].test(5));
}

TEST_CASE("Cover empty and clear", "[cover]") {
    Cover<64> cover;
    REQUIRE(cover.size() == 0);
    cover.add(BitCube<64>{0});
    REQUIRE(cover.size() == 1);
    cover = Cover<64>{};
    REQUIRE(cover.size() == 0);
}

TEST_CASE("BitCube serialization and deserialization", "[bitcube][serialize]") {
    BitCube<64> c1{0, 2, 5};
    std::string s = c1.to_string();
    BitCube<64> c2 = BitCube<64>::from_string(s);
    REQUIRE(c1 == c2);
    // Test invalid string
    REQUIRE_THROWS_AS(BitCube<64>::from_string("10a01"), std::invalid_argument);
    REQUIRE_THROWS_AS(BitCube<64>::from_string(std::string(200, '1')), std::invalid_argument); // too long
}

TEST_CASE("Cover serialization and deserialization", "[cover][serialize]") {
    Cover<64> cover1{BitCube<64>{0, 1}, BitCube<64>{2, 3}, BitCube<64>{4}};
    auto strings = cover1.to_strings();
    Cover<64> cover2 = Cover<64>::from_strings(strings);
    REQUIRE(cover1 == cover2);
    // Test with an invalid cube string
    std::vector<std::string> bad = strings;
    bad.push_back("10a01");
    REQUIRE_THROWS_AS(Cover<64>::from_strings(bad), std::invalid_argument);
}

TEST_CASE("BitCube inequality operator", "[bitcube]") {
    BitCube<64> a{1, 5, 7};
    BitCube<64> b{1, 5, 8};
    REQUIRE(a != b);
    b.set(7);
    b.set(8, false);
    REQUIRE(a == b);
}

TEST_CASE("Cover iterators", "[cover][iterators]") {
    BitCube<128> c1{0};
    BitCube<128> c2{1};
    BitCube<128> c3{2};
    Cover<128> cover{c1, c2, c3};

    auto it = cover.begin();
    REQUIRE(*it == c1);
    ++it;
    REQUIRE(*it == c2);
    ++it;
    REQUIRE(*it == c3);

    auto const_it = cover.begin();
    REQUIRE(*const_it == c1);
    ++const_it;
    REQUIRE(*const_it == c2);
    ++const_it;
    REQUIRE(*const_it == c3);
}

TEST_CASE("Cover iterator overflow", "[cover][iterators]") {
    BitCube<128> c1{0};
    BitCube<128> c2{1};
    BitCube<128> c3{2};
    Cover<128> cover{c1, c2, c3};

    auto it = cover.begin();
    ++it;
    ++it;
    ++it; // Move past the end
    REQUIRE(it == cover.end());

    auto const_it = cover.begin();
    ++const_it;
    ++const_it;
    ++const_it; // Move past the end
    REQUIRE(const_it == cover.end());
}
