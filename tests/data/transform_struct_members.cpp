#include <iostream>

struct Student {
    int marks = 90;

    void update() {
        int bonus = 5;
        marks += bonus;
    }
};

int main() {
    Student student;

    student.update();
    std::cout << student.marks << '\n';

    return 0;
}