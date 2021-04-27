#include <fstream>
#include <iostream>

#include "../include/tsv.hpp"


struct record
{
    unsigned row;
    unsigned column;
    double value;
};

int main()
{
    try {
        auto const records = tsv::load<record>(
            std::ifstream{"input.tsv"},
            {.header = false, .comment = '#'}
        );
        std::cout << records.size() << " records\n";
    } catch (tsv::error const& err) {
        std::cerr << "error: " << err.describe() << '\n';
    }
}
