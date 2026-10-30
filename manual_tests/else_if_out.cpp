#include <iostream>

int main() {
    if (int obf_var_1 = 10; obf_var_1 > 20) {
        std::cout << obf_var_1 << '\n';
    } else if (obf_var_1 > 5) {
        std::cout << obf_var_1 << '\n';
    }
    return 0;
}