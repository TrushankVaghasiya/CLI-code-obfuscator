
#include <iostream>

int main() {
    int outer = 10;

    {
        int inner = 20;
        std::cout << inner << '\n';
    }

    std::cout << outer << '\n';

    return 0;
}
