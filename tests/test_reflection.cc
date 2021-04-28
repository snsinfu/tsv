#include <string>
#include <type_traits>

#include <doctest.h>
#include <tsv.hpp>


TEST_CASE("record_size_v - detects number of aggregate fields")
{
    using tsv::detail::record_size_v;

    struct record_0 {};
    struct record_1 { int a1; };
    struct record_2 { int a1, a2; };
    struct record_3 { int a1, a2, a3; };

    static_assert(record_size_v<record_0> == 0);
    static_assert(record_size_v<record_1> == 1);
    static_assert(record_size_v<record_2> == 2);
    static_assert(record_size_v<record_3> == 3);
}

TEST_CASE("field_type_list - detects aggregate field types")
{
    using tsv::detail::type_list;
    using tsv::detail::field_type_list;

    SUBCASE("empty structure")
    {
        struct record {};
        using actual = field_type_list<record>;
        using expected = type_list<>;
        static_assert(std::is_same_v<actual, expected>);
    }

    SUBCASE("simple example")
    {
        struct record
        {
            int id;
            double value;
        };
        using actual = field_type_list<record>;
        using expected = type_list<int, double>;
        static_assert(std::is_same_v<actual, expected>);
    }

    SUBCASE("enum and class objects")
    {
        enum class record_type
        {
            red, black
        };
        struct record
        {
            int id;
            record_type type;
            std::string name;
        };
        using actual = field_type_list<record>;
        using expected = type_list<int, record_type, std::string>;
        static_assert(std::is_same_v<actual, expected>);
    }
}
