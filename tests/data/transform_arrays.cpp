#include <iostream>

int main() {
    int numbers[5] = {10, 20, 30, 40, 50};

    int total = 0;

    for (int i = 0; i < 5; i++) {
        total += numbers[i];
    }

    std::cout << total << '\n';

    return 0;
}