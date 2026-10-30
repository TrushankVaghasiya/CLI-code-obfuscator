#include <iostream>

int main() {
    int value = 10;

    if (int value = 20) {
        std::cout << value << '\n';

        while (int value = 30) {
            std::cout << value << '\n';
            break;
        }
    }

    std::cout << value << '\n';

    return 0;
}