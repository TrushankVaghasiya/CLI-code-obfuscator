#include <iostream>

int main() {
    int number = 10;
    int* ptr = &number;
    int& ref = number;

    *ptr = 20;

    std::cout << number << " " << ref << '\n';
    return 0;
}