#include <iostream>

int main() {
    int number = 7;
    char letter = 'A';
    const char* message = "Hello // world /* not a comment */";

    std::cout << number << '\n';
    std::cout << letter << '\n';
    std::cout << message << '\n';

    return 0;
}