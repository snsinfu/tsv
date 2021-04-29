#include <doctest.h>
#include <tsv.hpp>


TEST_CASE("load - parses valid tsv")
{
    struct record_type
    {
        unsigned row;
        unsigned column;
        double value;
    };

    std::istringstream input{
        "row\tcolumn\tvalue\n"
        "1\t2\t1.23\n"
        "3\t4\t4.56\n"
    };

    std::vector<record_type> const records = tsv::load<record_type>(input);

    CHECK(records.size() == 2);

    CHECK(records.at(0).row == 1);
    CHECK(records.at(0).column == 2);
    CHECK(records.at(0).value == doctest::Approx(1.23));

    CHECK(records.at(1).row == 3);
    CHECK(records.at(1).column == 4);
    CHECK(records.at(1).value == doctest::Approx(4.56));
}

TEST_CASE("load - accepts rvalue stream")
{
    struct record { int id; };
    tsv::load<record>(std::istringstream{"id"});
}
