#include <string_view>
#include <vector>

#include <doctest.h>
#include <tsv.hpp>


TEST_CASE("conversion - parses integral values")
{
    using value_type = int;

    SUBCASE("parse good examples")
    {
        struct example
        {
            std::string_view text;
            value_type expected;
        };

        std::vector<example> const examples = {
            {"1", 1},
            {"-1", -1},
            {"12345", 12345},
        };

        for (auto&& [text, expected] : examples) {
            auto const actual = tsv::conversion<value_type>::parse(text);
            CHECK(actual == expected);
        }
    }

    SUBCASE("catches errors")
    {
        std::vector<std::string> const examples = {
            "",
            "xxx",
            "123xxx",
        };

        for (auto&& text : examples) {
            CHECK_THROWS_AS(
                tsv::conversion<value_type>::parse(text), tsv::parse_error
            );
        }
    }
}

TEST_CASE("conversion - parses floating-point values")
{
    using value_type = double;

    SUBCASE("parses good examples")
    {
        struct example
        {
            std::string_view text;
            value_type expected;
        };

        std::vector<example> const examples = {
            {"0.1", 0.1},
            {"-0.1", -0.1},
            {"123.45", 123.45},
        };

        for (auto&& [text, expected] : examples) {
            auto const actual = tsv::conversion<value_type>::parse(text);
            CHECK(actual == doctest::Approx(expected));
        }
    }

    SUBCASE("catches errors")
    {
        std::vector<std::string> const examples = {
            "",
            "xxx",
            "123.45xxx",
        };

        for (auto&& text : examples) {
            CHECK_THROWS_AS(
                tsv::conversion<value_type>::parse(text), tsv::parse_error
            );
        }
    }
}

TEST_CASE("conversion - parses single character")
{
    using value_type = char;

    SUBCASE("parses good examples")
    {
        struct example
        {
            std::string_view text;
            value_type expected;
        };

        std::vector<example> const examples = {
            {"a", 'a'},
            {"b", 'b'},
        };

        for (auto&& [text, expected] : examples) {
            auto const actual = tsv::conversion<value_type>::parse(text);
            CHECK(actual == expected);
        }
    }

    SUBCASE("catches errors")
    {
        std::vector<std::string> const examples = {
            // Not single character
            "",
            "aa",
        };

        for (auto&& text : examples) {
            CHECK_THROWS_AS(
                tsv::conversion<value_type>::parse(text), tsv::parse_error
            );
        }
    }
}

TEST_CASE("conversion - parses string token")
{
    using value_type = std::string;

    SUBCASE("parses good examples")
    {
        struct example
        {
            std::string_view text;
            value_type expected;
        };

        std::vector<example> const examples = {
            {"", ""},
            {"abc", "abc"},
        };

        for (auto&& [text, expected] : examples) {
            auto const actual = tsv::conversion<value_type>::parse(text);
            CHECK(actual == expected);
        }
    }
}
