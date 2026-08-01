#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(
    const std::uint8_t* data, std::size_t size);

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "No scripting fuzz seeds were provided.\n";
        return 1;
    }

    for (int index = 1; index < argc; ++index) {
        std::ifstream stream(argv[index], std::ios::binary);
        if (!stream) {
            std::cerr << "Could not open fuzz seed: " << argv[index] << '\n';
            return 1;
        }
        const std::vector<std::uint8_t> input{
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()
        };
        LLVMFuzzerTestOneInput(input.data(), input.size());
    }
    return 0;
}
