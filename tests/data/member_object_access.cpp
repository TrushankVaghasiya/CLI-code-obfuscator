class Counter {
public:
    int count = 10;
};

int main() {
    Counter counter;

    counter.count++;

    Counter* ptr = &counter;
    ptr->count++;

    return 0;
}