#include <iostream>

int main() {
    int value = 10;

    auto print = [&]() {
        int value = 20;
        std::cout << value << '\n';
    };

    std::cout << value << '\n';
    print();

    return 0;
}