#include <iostream>

int globalCount = 100;

void display() {
    std::cout << globalCount << '\n';
}

int main() {
    int localCount = 50;

    globalCount += localCount;

    display();

    return 0;
}