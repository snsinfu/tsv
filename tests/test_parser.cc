#include <sstream>
#include <string>
#include <vector>

#include <doctest.h>
#include <tsv.hpp>


namespace
{
    struct my_rational
    {
        int numerator;
        int denominator;
    };

    std::istream& operator>>(std::istream& is, my_rational& rat)
    {
        char delim = 0;
        is >> rat.numerator >> delim >> rat.denominator;
        if (!is || delim != '/') {
            is.setstate(std::ios::failbit);
        }
        return is;
    }
}

TEST_CASE("parser::parse_fields")
{
    SUBCASE("non-uniform records")
    {
        std::istringstream source{
            "first\trecord\n"
            "second\trecord\textra field\n"
        };
        tsv::detail::parser parser{source, '\t'};

        std::vector<std::string> first_record;
        CHECK(parser.parse_fields(first_record));
        CHECK(first_record == std::vector<std::string>{"first", "record"});

        std::vector<std::string> second_record;
        CHECK(parser.parse_fields(second_record));
        CHECK(second_record == std::vector<std::string>{"second", "record", "extra field"});

        // Returns false on encountering EOF.
        std::vector<std::string> no_record;
        CHECK_FALSE(parser.parse_fields(no_record));
    }

    SUBCASE("empty fields")
    {
        std::istringstream source{"\t\t"};
        tsv::detail::parser parser{source, '\t'};

        std::vector<std::string> record;
        CHECK(parser.parse_fields(record));
        CHECK(record == std::vector<std::string>{"", "", ""});
    }

    SUBCASE("empty input")
    {
        std::istringstream source{""};
        tsv::detail::parser parser{source, '\t'};

        std::vector<std::string> no_record;
        CHECK_FALSE(parser.parse_fields(no_record));
    }
}

TEST_CASE("parser::skip_comment")
{
    SUBCASE("input containing comment lines")
    {
        std::istringstream source{
            "first\trecord\n"
            "# comment\n"
            "# comment\n"
            "second\trecord\n"
        };
        tsv::detail::parser parser{source, '\t'};

        // This should skip nothing.
        parser.skip_comment('#');

        std::vector<std::string> first_record;
        CHECK(parser.parse_fields(first_record));
        CHECK(first_record == std::vector<std::string>{"first", "record"});

        // This should skip the "# comment" lines.
        parser.skip_comment('#');

        std::vector<std::string> second_record;
        CHECK(parser.parse_fields(second_record));
        CHECK(second_record == std::vector<std::string>{"second", "record"});
    }

    SUBCASE("input containing comment lines with different prefix")
    {
        std::istringstream source{
            "#111111\n"
            "! comment\n"
            "#222222\n"
        };
        tsv::detail::parser parser{source, '\t'};

        std::vector<std::string> first_record;
        CHECK(parser.parse_fields(first_record));
        CHECK(first_record == std::vector<std::string>{"#111111"});

        // This should skip the "! comment" lines.
        parser.skip_comment('!');

        std::vector<std::string> second_record;
        CHECK(parser.parse_fields(second_record));
        CHECK(second_record == std::vector<std::string>{"#222222"});
    }

    SUBCASE("input containing empty lines")
    {
        std::istringstream source{
            "first\trecord\n"
            "\n"
            "\n"
            "second\trecord\n"
        };
        tsv::detail::parser parser{source, '\t'};

        std::vector<std::string> first_record;
        CHECK(parser.parse_fields(first_record));
        CHECK(first_record == std::vector<std::string>{"first", "record"});

        // This should skip the empty lines. There is no comment line in the
        // input but empty lines are always skipped.
        parser.skip_comment('#');

        std::vector<std::string> second_record;
        CHECK(parser.parse_fields(second_record));
        CHECK(second_record == std::vector<std::string>{"second", "record"});
    }

    SUBCASE("works with empty input")
    {
        std::istringstream source{""};
        tsv::detail::parser parser{source, '\t'};
        parser.skip_comment('#');
        CHECK(source.eof());
    }
}

TEST_CASE("parser::parse_record")
{
    SUBCASE("valid input with standard types")
    {
        struct record_type
        {
            unsigned row;
            unsigned column;
            double value;
            std::string label;
        };

        std::istringstream source{
            "0\t1\t1.23\tID_01\n"
            "2\t3\t4.56\tID_23\n"
        };
        tsv::detail::parser parser{source, '\t'};

        record_type first_record;
        CHECK(parser.parse_record(first_record));
        CHECK(first_record.row == 0u);
        CHECK(first_record.column == 1u);
        CHECK(first_record.value == doctest::Approx(1.23));
        CHECK(first_record.label == "ID_01");

        record_type second_record;
        CHECK(parser.parse_record(second_record));
        CHECK(second_record.row == 2u);
        CHECK(second_record.column == 3u);
        CHECK(second_record.value == doctest::Approx(4.56));
        CHECK(second_record.label == "ID_23");

        record_type no_record;
        CHECK_FALSE(parser.parse_record(no_record));
    }

    SUBCASE("valid input with custom type")
    {
        struct record_type
        {
            my_rational value;
            std::string name;
        };

        std::istringstream source{
            "1/137\tfine structure constant\n"
            "22/7\tpi\n"
        };
        tsv::detail::parser parser{source, '\t'};

        record_type first_record;
        CHECK(parser.parse_record(first_record));
        CHECK(first_record.value.numerator == 1);
        CHECK(first_record.value.denominator == 137);
        CHECK(first_record.name == "fine structure constant");

        record_type second_record;
        CHECK(parser.parse_record(first_record));
        CHECK(first_record.value.numerator == 22);
        CHECK(first_record.value.denominator == 7);
        CHECK(first_record.name == "pi");

        record_type no_record;
        CHECK_FALSE(parser.parse_record(no_record));
    }

    SUBCASE("empty input")
    {
        struct record_type
        {
        };

        std::istringstream source{""};
        tsv::detail::parser parser{source, '\t'};

        record_type record;
        CHECK_FALSE(parser.parse_record(record));
    }

    SUBCASE("errors")
    {
        struct record_type
        {
            unsigned source;
            unsigned destination;
        };

        std::vector<std::string> const examples = {
            // Missing field
            "123",

            // Extra field
            "123\t456\t",
            "123\t456\t789",

            // Out of range
            "123\t-456",
            "123\t9999999999999999999999999999999999999999999999999999",

            // Parse error
            "123\t4.56",
            "source\tdestination",
            "# comment",
        };

        for (auto&& example : examples) {
            std::istringstream source{example};
            tsv::detail::parser parser{source, '\t'};

            record_type record;
            INFO("source = \"", example, "\"");
            CHECK_THROWS_AS(parser.parse_record(record), tsv::error);
        }
    }

}
