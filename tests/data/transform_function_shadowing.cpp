int calculate(int number) {
    int result = number * 2;

    {
        int number = 100;
        result += number;
    }

    return result + number;
}