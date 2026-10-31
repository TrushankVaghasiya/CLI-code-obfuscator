#include <iostream>

int calculate(int value) {
    auto operation = [](int value) {
        return value * 2;
    };

    return operation(value);
}

int main() {
    std::cout << calculate(10) << '\n';
    return 0;
}