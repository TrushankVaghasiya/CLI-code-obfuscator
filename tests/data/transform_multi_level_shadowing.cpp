#include <iostream>

int main() {
    int value = 10;

    {
        int value = 20;

        {
            int value = 30;
            std::cout << value << '\n';
        }

        std::cout << value << '\n';
    }

    std::cout << value << '\n';

    return 0;
}