#include <iostream>

int main() {
    int value = 10;

    auto print = [&]() {
        std::cout << value << '\n';
    };

    print();

    return 0;
}