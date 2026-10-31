int calculate(int x) {
    return x * 2;
}

int main() {
    auto calculate = [](int x) {
        return x + 10;
    };

    return calculate(5);
}