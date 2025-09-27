#include <array>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

// ABOUT UPDATES

const int N_UPDATES = 1'000'000;

struct Update { int8_t op; int x, y; }; // y is only for '+'

alignas(64) Update upds[N_UPDATES];
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

const u_int16_t BUCKET_SZ1 = 15;
const u_int16_t BUCKET_SZ2 = BUCKET_SZ1 * BUCKET_SZ1;
const u_int16_t MAX_PRICE  = BUCKET_SZ1 * BUCKET_SZ1 * BUCKET_SZ1;
const u_int16_t QUEUE_CAP  = 256;

class queue {
    alignas(64) std::array<u_int16_t, QUEUE_CAP> arr{}; // alignas(64) for cache line
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

        // u_int8_t loc = st + pos;
        // if (loc < en) {
        //     size_t len = en - loc - 1;
        //     if (len) memmove(&arr[loc], &arr[(u_int8_t)(loc + 1)], len * sizeof(arr[0]));
        // } else {
        //     {
        //         size_t len = QUEUE_CAP - loc - 1;
        //         if (len) memmove(&arr[loc], &arr[(u_int8_t)(loc + 1)], len * sizeof(arr[0]));
        //     }
        //     if (en) {
        //         arr[QUEUE_CAP - 1] = arr[0];

        //         size_t len = en - 1;
        //         if (len) memmove(&arr[0], &arr[(u_int8_t)1], len * sizeof(arr[0]));
        //     }
        // }
        // en--, siz--;
    }
};

std::array<queue, MAX_PRICE> ob;
std::array<u_int16_t, BUCKET_SZ1> ob_bu1;
std::array<u_int16_t, BUCKET_SZ2> ob_bu2;

// ABOUT SOLVE

inline void op0(int price, int size) {
    ob[price].push(size);
    ob_bu1[price / BUCKET_SZ2]++;
    ob_bu2[price / BUCKET_SZ1]++;
}
inline void op1(int pos) {
    int i;
    for (i = 0; ; i++) {
        if (pos < ob_bu1[i]) break;
        else pos -= ob_bu1[i];
    }
    for (i *= BUCKET_SZ1; ; i++) {
        if (pos < ob_bu2[i]) break;
        else pos -= ob_bu2[i];
    }
    for (int price = i * BUCKET_SZ1; ; price++) {
        if (pos < ob[price].size()) {
            ob[price].erase(pos);
            ob_bu1[price / BUCKET_SZ2]--;
            ob_bu2[price / BUCKET_SZ1]--;
            break;
        } else pos -= ob[price].size();
    }
}
inline void op2(int size) {
    int i;
    for (i = 0; ; i++) {
        if (ob_bu1[i]) break;
    }
    for (i *= BUCKET_SZ1; ; i++) {
        if (ob_bu2[i]) break;
    }

    for (int price = i * BUCKET_SZ1; size > 0; price++) {
        while (ob[price].size() && size > 0) {
            u_int16_t &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            if (x == 0) {
                ob[price].pop();
                ob_bu1[price / BUCKET_SZ2]--;
                ob_bu2[price / BUCKET_SZ1]--;
            }
        }
    }
}
inline int op3(int size) {
    int ret = 0;

    int i;
    for (i = 0; ; i++) {
        if (ob_bu1[i]) break;
    }
    for (i *= BUCKET_SZ1; ; i++) {
        if (ob_bu2[i]) break;
    }

    for (int price = i * BUCKET_SZ1; size > 0; price++) {
        while (ob[price].size() && size > 0) {
            u_int16_t &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            ret += mn * price;
            if (x == 0) {
                ob[price].pop();
                ob_bu1[price / BUCKET_SZ2]--;
                ob_bu2[price / BUCKET_SZ1]--;
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
