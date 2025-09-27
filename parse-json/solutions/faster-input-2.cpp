#include <iostream>
#include <string>
#include <array>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <tuple>
#include <cstring>

void input(char **in) {
    struct stat st{};
    fstat(STDIN_FILENO, &st);
    size_t size = st.st_size;

    *in = new char[(int)(92085065 * 1.1)];
    
    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, STDIN_FILENO, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    const char *s = (const char *) ptr;
    const char *e = s + size;

    char *acc = *in;

    *acc++ = ' ';
    while (s < e) {
        char c = *s++;

        if (c == '{' || c == '}' || c == ':' || ('0' <= c && c <= '9') || c == 'd' || c == 't' || c == 'S' || c == ',') *acc++ = c;
    }
    *acc++ = ' ';
    *acc++ = '\0';
}

void solve(const char *in, u_int64_t &ans) {
    int stk = 0;
    int n = strlen(in);
    
    std::array<std::tuple<int, int>, 10> transactions;
    int user_id = 0;
    bool is_usd = 0;
    int idx = 0;

    int to_user_id = 0, amount = 0;
    bool canceled = 0;
    
    for (int i = 1; i + 1 < n; i++) {
        char c = in[i], cn1 = in[i + 1], cp1 = in[i - 1];
        if (c == '{') {
            stk++;
        } else if (c == '}') {
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
        } else if (c == ':' && '0' <= cn1 && cn1 <= '9') {
            int tmp = 0;
            for (i++; '0' <= in[i] && in[i] <= '9'; i++) {
                tmp = 10 * tmp + in[i] - '0';
            }
            i--;

            if (stk == 1) {
                user_id = tmp;
            } else {
                cp1 == 'd' ? to_user_id = tmp : amount = tmp;
            }
        } else if (stk == 2 && c == ':' && cn1 == 't') {
            canceled = 1;
        } else if (c == 'S') {
            is_usd = 1;
        }
    }
}

int main() {
    char *in;
    input(&in);

    u_int64_t ans = 0;
    solve(in, ans);

    std::cout << ans << '\n';
}
