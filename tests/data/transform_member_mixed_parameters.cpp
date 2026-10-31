class Counter {
public:
    int count = 10;
};

void update(Counter first, Counter* second, Counter& third) {
    first.count++;
    second->count++;
    third.count++;
}

int main() {
    Counter a, b, c;
    update(a, &b, c);
    return 0;
}