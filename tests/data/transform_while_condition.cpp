#include <iostream>

int main() {
    int count = 3;

    while (int value = count) {
        std::cout << value << '\n';
        --count;
    }

    return 0;
}