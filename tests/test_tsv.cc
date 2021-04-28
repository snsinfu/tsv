#include <doctest.h>
#include <tsv.hpp>


TEST_CASE("load - accepts rvalue stream")
{
    struct record { int id; };
    tsv::load<record>(std::istringstream{"id"});
}
