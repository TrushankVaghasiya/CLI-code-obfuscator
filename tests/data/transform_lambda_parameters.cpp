#include <iostream>

int main() {
    int value = 10;

    auto print = [](int value) {
        std::cout << value << '\n';
    };

    std::cout << value << '\n';
    print(20);

    return 0;
}