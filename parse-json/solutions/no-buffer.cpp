#include <iostream>
#include <string>
#include <array>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <tuple>
#include <cstring>

int main() {
    struct stat st{};
    fstat(STDIN_FILENO, &st);
    size_t size = st.st_size;

    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, STDIN_FILENO, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    const char *s = (const char *) ptr;
    const char *e = s + size;

    u_int64_t ans = 0;

    int stk = 0;
    
    std::array<std::tuple<int, int>, 10> transactions;
    int user_id = 0;
    bool is_usd = 0;
    int idx = 0;

    int to_user_id = 0, amount = 0;
    bool canceled = 0;

    char cn, cp1, cp2;
    while (s < e) {
        char c = *s++;

        if (!(c == '{' || c == '}' || c == ':' || ('0' <= c && c <= '9') || c == 'd' || c == 't' || c == 'S' || c == ',')) continue;

        cp2 = cp1, cp1 = cn, cn = c;

        if (cn == '{') {
            stk++;
        } else if (cn == '}') {
            stk--;
            if (stk == 0) {
                if (is_usd) {
                    for (int j = 0; j < idx; j++) {
                        ans += (std::get<0>(transactions[j]) != user_id) * std::get<1>(transactions[j]);
                    }
                }
                user_id = is_usd = idx = 0;
            } else if (stk == 1) {
                if (!canceled) {
                    transactions[idx++] = {to_user_id, amount};
                }
                to_user_id = amount = canceled = 0;
            }
        } else if (cp1 == ':' && '0' <= cn && cn <= '9') {
            int tmp = cn - '0';
            for (; '0' <= *s && *s <= '9'; s++) {
                tmp = 10 * tmp + *s - '0';
            }
            s--;

            if (stk == 1) {
                user_id = tmp;
            } else {
                cp2 == 'd' ? to_user_id = tmp : amount = tmp;
            }
        } else if (stk == 2 && cp1 == ':' && cn == 't') {
            canceled = 1;
        } else if (cn == 'S') {
            is_usd = 1;
        }
    }

    std::cout << ans << '\n';
}
