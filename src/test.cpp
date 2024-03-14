#include "expr.hpp"

#include <doctest/doctest.h>

#include <sstream>

constexpr void check_binary() {
    state_t state = std::vector{2.0, 3.0, 0.0};

    static_assert((constant_t(2.0) + 4.0)(state) == 6);
    static_assert((constant_t(2.0) - 4.0)(state) == -2);
    static_assert((constant_t(2.0) * 4.0)(state) == 8);
    static_assert((constant_t(2.0) / 4.0)(state) == 0.5);

    static_assert((-((constant_t(2.0) + 4.0)))(state) == -6);

}

TEST_CASE("Calculate")
{
    auto sys = symbol_table_t{};
    auto a = sys.variable("a", 2);
    auto b = sys.variable("b", 3);
    auto c = sys.variable("c", 0);

    auto& state = sys.m_state;


    SUBCASE("Reading the value of a variable from state")
    {
        CHECK(a(state) == 2);
        CHECK(b(state) == 3);
        CHECK(c(state) == 0);
    }
    SUBCASE("Unary operations")
    {
        CHECK((+a)(state) == 2);
        CHECK((-b)(state) == -3);
        CHECK((-c)(state) == 0);
    }
    SUBCASE("Addition and subtraction")
    {
        CHECK((a + b)(state) == 5);
        CHECK((a - b)(state) == -1);
        // the state should not have changed:
        CHECK(a(state) == 2);
        CHECK(b(state) == 3);
        CHECK(c(state) == 0);
    }
    SUBCASE("Assignment expression evaluation")
    {
        CHECK(c(state) == 0);
        CHECK((c <<= b - a)(state) == 1);
        CHECK(c(state) == 1);
        CHECK((c += b - a * c)(state) == 2);
        CHECK(c(state) == 2);
        CHECK((c += b - a * c)(state) == 1);
        CHECK(c(state) == 1);

        // Compile time error: assignment expressions only defined for var_t's
        //CHECK_THROWS_MESSAGE((c - a += b - c), "assignment destination must be a variable expression");
    }
    SUBCASE("Parenthesis")
    {
        CHECK((a - (b - c))(state) == -1);
        CHECK((a - (b - a))(state) == 1);
    }

    SUBCASE("Evaluation of multiplication and division")
    {
        CHECK((a * b)(state) == 6);
        CHECK((a / b)(state) == 2. / 3);
        CHECK_THROWS_MESSAGE((a / c)(state), "division by zero");
    }
    SUBCASE("Mixed addition and multiplication")
    {
        CHECK((a + a * b)(state) == 8);
        CHECK((a - b / a)(state) == 0.5);
    }
    SUBCASE("Constant expressions")
    {
        CHECK((7 + a)(state) == 9);
        CHECK((a - 7)(state) == -5);
    }
    SUBCASE("Store expression and evaluate lazily")
    {
        auto expr = (a + b) * c;
        auto c_4 = c <<= 4;
        CHECK(expr(state) == 0);
        CHECK(c_4(state) == 4);
        CHECK(expr(state) == 20);
    }
    SUBCASE("Printing") {
        std::stringstream ss;
        SUBCASE("a+b") {
            ss << printer{sys, a + b};
            CHECK(ss.str() == "a+b");
        }

        SUBCASE("a+=b") {
            ss << printer{sys, a += b};
            CHECK(ss.str() == "a+=b");
        }

        SUBCASE("a<<=b") {
            ss << printer{sys, a <<= b};
            CHECK(ss.str() == "a<<=b");
        }

        SUBCASE("a+2") {
            ss << printer{sys, a + 2};
            CHECK(ss.str() == "a+2");
        }
    }
}