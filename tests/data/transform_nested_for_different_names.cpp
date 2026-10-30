#include <iostream>

int main() {
    int x = 100;

    for (int i = 0; i < 3; ++i) {
        int j = i * 2;

        for (int k = 0; k < 2; ++k) {
            std::cout << x + i + j + k << '\n';
        }

        std::cout << j << '\n';
    }

    return 0;
}