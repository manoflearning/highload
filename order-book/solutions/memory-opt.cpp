#include <array>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cassert>

// ABOUT UPDATES

const int N_UPDATES = 1'000'000;

struct Update { int op, x, y; }; // y is only for '+'

Update upds[N_UPDATES];
size_t upds_idx;

void input() {
    off_t size = lseek(0, 0, SEEK_END);
    void *ptr = mmap(0, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, 0, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    const char *s = (const char *)ptr;
    const char *e = s + size;

    while (s < e) {
        char c = *s;

        if ((unsigned)(c - '0') <= 9) {
            int tmp = *s++ - '0';
            while ((unsigned)(*s - '0') <= 9) {
                tmp = 10 * tmp + *s++ - '0';
            }
            upds[upds_idx].x == 0 ? upds[upds_idx].x = tmp : upds[upds_idx].y = tmp;
        } else if (c == '+' || c == '-' || c == '=') {
            upds[upds_idx].op = (c == '+' ? 0 : (c == '-' ? 1 : 2));
            s++;
        } else if (c == '\n') {
            upds_idx++;
            s++;
        } else { s++; }
    }

    assert(upds_idx == N_UPDATES);
}

// ABOUT ORDER BOOK

const u_int16_t SQ        = 55;
const u_int16_t MAX_PRICE = SQ * SQ;
const u_int16_t N_BUCKET  = MAX_PRICE / SQ;
const u_int16_t QUEUE_CAP = 256;

class queue {
    std::array<u_int16_t, QUEUE_CAP> arr{};
    u_int8_t st{}, en{}, siz{};
public:
    void push(int x) { arr[en++] = x, siz++; }
    void pop() { st++, siz--; }
    u_int16_t &front() { return arr[st]; }
    u_int8_t size() { return siz; }
    void erase(u_int8_t pos) {
        if ((int)pos * 2 <= size()) {
            for (u_int8_t i = st + pos; i != st; i--) {
                arr[i] = arr[(u_int8_t)(i - 1)];
            }
            st++, siz--;
        } else {
            for (u_int8_t i = st + pos + 1; i != en; i++) {
                arr[(u_int8_t)(i - 1)] = arr[i];
            }
            en--, siz--;
        }
    }
};

std::array<queue, MAX_PRICE> ob;
std::array<int, N_BUCKET> ob_bu;

// ABOUT SOLVE

inline void op0(int price, int size) {
    ob[price].push(size);
    ob_bu[price / SQ]++;
}
inline void op1(int pos) {
    int i;
    for (i = 0; i < N_BUCKET; i++) {
        if (pos < ob_bu[i]) break;
        pos -= ob_bu[i];
    }
    for (int price = i * SQ; price < MAX_PRICE; price++) {
        if (pos < ob[price].size()) {
            ob[price].erase(pos);
            ob_bu[price / SQ]--;
            break;
        }
        pos -= ob[price].size();
    }
}
inline void op2(int size) {
    int i;
    for (i = 0; i < N_BUCKET; i++) {
        if (ob_bu[i]) break;
    }

    for (int price = i * SQ; price < MAX_PRICE && size > 0; price++) {
        while (ob[price].size() && size > 0) {
            u_int16_t &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            if (x == 0) {
                ob[price].pop();
                ob_bu[price / SQ]--;
            }
        }
    }
}
inline int op3(int size) {
    int ret = 0;

    int i;
    for (i = 0; i < N_BUCKET; i++) {
        if (ob_bu[i]) break;
    }

    for (int price = i * SQ; price < MAX_PRICE && size > 0; price++) {
        while (ob[price].size() && size > 0) {
            u_int16_t &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            ret += mn * price;
            if (x == 0) {
                ob[price].pop();
                ob_bu[price / SQ]--;
            }
        }
    }

    return ret;
}

void solve() {
    for (int i = 0; i < upds_idx; i++) {
        auto &it = upds[i];
        if (it.op == 0) op0(it.x, it.y);
        else if (it.op == 1) op1(it.x);
        else op2(it.x);
    }
}

int main() {
    input();

    solve();

    std::cout << op3(1000) << '\n';
}
