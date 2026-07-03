#include "BCryptHasher.h"
#include <iostream>
int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: gen_hash <password> [cost]" << std::endl; return 1; }
    unsigned int cost = argc > 2 ? (unsigned int)std::stoi(argv[2]) : 10;
    std::string h = BCryptHasher::generateHash(argv[1], cost);
    std::cout << h << std::endl;
    return 0;
}
