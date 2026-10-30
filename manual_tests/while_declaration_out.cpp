#include <iostream>

int main() {
    int obf_var_1 = 3;

    while (int obf_var_2 = obf_var_1) {
        std::cout << obf_var_2 << '\n';
        --obf_var_1;
    }

    return 0;
}