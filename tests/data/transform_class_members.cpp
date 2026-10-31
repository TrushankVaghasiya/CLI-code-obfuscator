#include <iostream>

class Counter {
public:
    int count = 10;

    void increment() {
        int value = 5;
        count += value;
    }

    void display() {
        std::cout << count << '\n';
    }
};

int main() {
    Counter counter;

    counter.increment();
    counter.display();

    return 0;
}