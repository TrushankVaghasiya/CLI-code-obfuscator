#include <iostream>

int main() {
    int i = 100;

    for (int i = 0; i < 3; ++i) {
        std::cout << i << '\n';

        for (int i = 0; i < 2; ++i) {
            std::cout << i << '\n';
        }

        std::cout << i << '\n';
    }

    std::cout << i << '\n';

    return 0;
}