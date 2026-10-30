#include <iostream>

int main() {
    if (int value = 10; value > 20) {
        std::cout << value << '\n';
    } else if (value > 5) {
        std::cout << value << '\n';
    }
    return 0;
}