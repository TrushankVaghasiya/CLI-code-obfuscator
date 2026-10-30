#include <iostream>

int main() {
    int count = 3;
    int multiplier = 2;

    while (int value = count) {
        std::cout << value * multiplier << '\n';
        --count;
    }

    return 0;
}