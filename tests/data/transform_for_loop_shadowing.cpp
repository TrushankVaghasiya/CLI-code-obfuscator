#include <iostream>

int main() {
    for (int i = 0; i < 5; i++) {
        std::cout << i << '\n';

        {
            int i = 100;
            std::cout << i << '\n';
        }

        std::cout << i << '\n';
    }

    return 0;
}