int calculate(int a) {
    return a * 2;
}

double calculate(double a) {
    return a * 3.0;
}

int main() {
    int x = calculate(5);
    double y = calculate(2.5);

    return x + static_cast<int>(y);
}