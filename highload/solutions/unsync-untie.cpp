#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    u_int64_t sum = 0;
    u_int64_t x = 0;
    while (std::cin >> x) sum += x;
    std::cout << sum;
}
