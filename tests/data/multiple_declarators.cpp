#include <iostream>

int main() {
    int first = 10, second = 20, third = 30;

    first++;
    second += first;
    third = second + first;

    std::cout << first << " "
              << second << " "
              << third << std::endl;

    return 0;
}