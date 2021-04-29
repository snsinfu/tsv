#include <sstream>

#include <doctest.h>
#include <tsv.hpp>


TEST_CASE("line_reader::consume")
{
    using tsv::detail::line_reader;

    SUBCASE("reads lines from normal input")
    {
        std::istringstream source{
            "first line\n"
            "second line\n"
        };
        line_reader reader{source};

        CHECK(reader.consume() == "first line");
        CHECK(reader.consume() == "second line");
        CHECK(reader.consume() == std::nullopt);
    }

    SUBCASE("reads nothing from empty input")
    {
        std::istringstream source{""};
        line_reader reader{source};

        CHECK(reader.consume() == std::nullopt);
    }
}

TEST_CASE("line_reader::peek")
{
    using tsv::detail::line_reader;

    SUBCASE("reads unconsumed lines from normal input")
    {
        std::istringstream source{
            "first line\n"
            "second line\n"
        };
        line_reader reader{source};

        CHECK(reader.peek() == "first line");
        CHECK(reader.peek() == "first line");
        CHECK(reader.consume() == "first line");
        CHECK(reader.peek() == "second line");
        CHECK(reader.peek() == "second line");
        CHECK(reader.consume() == "second line");
        CHECK(reader.peek() == std::nullopt);
    }

    SUBCASE("reads nothing from empty input")
    {
        std::istringstream source{""};
        line_reader reader{source};

        CHECK(reader.peek() == std::nullopt);
    }
}

TEST_CASE("line_reader::line_number - returns the number of lines read")
{
    using tsv::detail::line_reader;

    std::istringstream source{
        "first line\n"
        "second line\n"
    };
    line_reader reader{source};

    CHECK(reader.line_number() == 0);

    CHECK(reader.consume() == "first line");
    CHECK(reader.line_number() == 1);

    CHECK(reader.consume() == "second line");
    CHECK(reader.line_number() == 2);

    CHECK(reader.consume() == std::nullopt);
    CHECK(reader.line_number() == 2);
}
