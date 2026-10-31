class Counter {
public:
    int count = 10;
};

void update(Counter counter) {
    counter.count++;

    {
        int counter = 5;
        counter++;
    }

    counter.count++;
}

int main() {
    Counter counter;
    counter.count++;

    update(counter);

    return 0;
}
