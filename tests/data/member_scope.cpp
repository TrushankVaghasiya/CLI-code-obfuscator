class Counter {
public:
    int count = 10;
};

int main() {
    Counter counter;

    counter.count++;

    {
        int counter = 5;
        counter++;
    }

    counter.count++;

    return 0;
}