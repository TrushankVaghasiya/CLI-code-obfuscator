class Counter {
public:
    int count = 10;

    void increment() {
        count++;
    }

    void incrementExplicitly() {
        this->count++;
    }
};

int main() {
    Counter counter;
    counter.count++;
}