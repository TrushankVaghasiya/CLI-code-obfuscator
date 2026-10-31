class Counter {
public:
    int count = 10;
};

void update(Counter first, Counter second) {
    first.count++;
    second.count++;
}

int main() {
    Counter a, b;
    update(a, b);
    return 0;
}