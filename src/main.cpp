#include "CppReferenceExtractor.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout
            << "Usage:\n"
            << "CppReferenceExtractor input.html output.md\n";

        return 1;
    }

    CppReferenceExtractor extractor;

    if (!extractor.Convert(argv[1], argv[2]))
    {
        std::cerr << "Conversion failed.\n";
        return 1;
    }

    std::cout << "Done.\n";

    return 0;
}