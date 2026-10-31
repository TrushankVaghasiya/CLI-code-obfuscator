class Counter {
public:
    int count = 10;
};

int main() {
    Counter counter;

    auto update = [](Counter counter) {
        counter.count++;
    };

    update(counter);
    return 0;
}