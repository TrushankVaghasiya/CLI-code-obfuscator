#include <iostream>

int main() {
    int total = 0;

    for (int i = 0; i < 5; i++) {
        total += i;
    }

    std::cout << total << '\n';
    return 0;
}