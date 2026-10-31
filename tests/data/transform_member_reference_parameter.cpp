class Counter {
public:
    int count = 10;
};

void update(Counter& counter) {
    counter.count++;
}

int main() {
    Counter counter;
    update(counter);
    return 0;
}