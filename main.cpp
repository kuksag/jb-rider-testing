#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <map>

int main() {
    const char FILE_PATH[] = "words.txt";

    std::ifstream file(FILE_PATH, std::ifstream::in);
    assert(file.good());

    std::string line;

    std::map<std::size_t, int> counter;

    while (file >> line) {
        counter[line.size()]++;
    }
    for (std::pair<std::size_t, int> item : counter) {
        std::cout << item.first << " " << item.second << '\n';
    }
}
