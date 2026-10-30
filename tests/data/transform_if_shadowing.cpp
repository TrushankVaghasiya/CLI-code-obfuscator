#include <iostream>

int main() {
    if (int value = 10; value > 5) {
        std::cout << value << '\n';

        {
            int value = 20;
            std::cout << value << '\n';
        }

        std::cout << value << '\n';
    }

    return 0;
}