int add(int a, int b);  // Function prototype

int multiply(int a, int b) {
    return a * b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }

    return n * factorial(n - 1);  // Recursive call
}

int add(int a, int b) {  // Function definition
    return a + b;
}

int main() {
    int result1 = add(10, 20);
    int result2 = multiply(4, 5);
    int result3 = factorial(5);

    return result1 + result2 + result3;
}