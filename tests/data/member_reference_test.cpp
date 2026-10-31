class Counter {
public:
    int count = 10;

    void increment() {
        count++;
    }

    void incrementExplicitly() {
        this->count++;
    }

    void testParameter(int count) {
        count++;
        this->count++;
    }

    void testLocal() {
        int count = 5;
        count++;
        this->count++;
    }

    void testNestedScope() {
        count++;

        {
            int count = 20;
            count++;
        }

        count++;
    }
};

int main() {
    Counter counter;
    counter.count++;
}