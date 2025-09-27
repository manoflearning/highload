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

int parse_int(const char *&p) {
    int ret = *p++ - '0';
    while ((unsigned)(*p - '0') <= 9) {
        ret = 10 * ret + *p++ - '0';
    }
    return ret;
}

void input() {
    off_t size = lseek(0, 0, SEEK_END);
    void *ptr = mmap(0, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, 0, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    const char *s = (const char *)ptr;
    const char *e = s + size;
    
    while (s < e) {
        const char *line = (const char *)memchr(s, '\n', e - s);
        if (!line) break;
        
        const char *p = s;

        char c = *p;
        if (c == '+') {
            upds[upds_idx].op = 0;
            p += 2;
            upds[upds_idx].x = parse_int(p);
            p += 1;
            upds[upds_idx].y = parse_int(p);
        } else if (c == '-') {
            upds[upds_idx].op = 1;
            p += 2;
            upds[upds_idx].x = parse_int(p);
        } else {
            upds[upds_idx].op = 2;
            p += 2;
            upds[upds_idx].x = parse_int(p);
        }

        s = line + 1;
        upds_idx++;

        // char c = *s;

        // if ((unsigned)(c - '0') <= 9) { 
        //     int tmp = *s++ - '0';
        //     while ((unsigned)(*s - '0') <= 9) {
        //         tmp = 10 * tmp + *s++ - '0';
        //     }
        //     upds[upds_idx].x == 0 ? upds[upds_idx].x = tmp : upds[upds_idx].y = tmp;
        // } else if (c == '+' || c == '-' || c == '=') {
        //     upds[upds_idx].op = (c == '+' ? 0 : (c == '-' ? 1 : 2));
        //     s++;
        // } else if (c == '\n') {
        //     upds_idx++;
        //     s++;
        // } else { s++; }
    }

    assert(upds_idx == N_UPDATES);
}

// ABOUT ORDER BOOK

const u_int16_t BUCKET     = 7;
const u_int16_t BIT        = 64;
const u_int16_t MAX_PRICE  = BUCKET * BUCKET * BIT;
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
std::array<u_int16_t, BUCKET> ob_bu1;
std::array<u_int16_t, BUCKET * BUCKET> ob_bu2;
std::array<u_int64_t, BUCKET * BUCKET> ob_bit;

// ABOUT SOLVE

inline void op0(int price, int size) {
    ob[price].push(size);
    ob_bu1[price / (BUCKET * BIT)]++;
    ob_bu2[price / BIT]++;
    ob_bit[price / BIT] |= (1ll << (price % BIT));
}
inline void op1(int pos) {
    int i;
    for (i = 0; ; i++) {
        if (pos < ob_bu1[i]) break;
        else pos -= ob_bu1[i];
    }
    for (i *= BUCKET; ; i++) {
        if (pos < ob_bu2[i]) break;
        else pos -= ob_bu2[i];
    }
    u_int64_t bit = ob_bit[i];
    while (bit) {
        int j = __builtin_ctzll(bit);
        bit ^= (1ll << j);

        int price = i * BIT + j;

        if (pos < ob[price].size()) {
            ob[price].erase(pos);
            ob_bu1[price / (BUCKET * BIT)]--;
            ob_bu2[price / BIT]--;
            if (!ob[price].size()) ob_bit[price / BIT] ^= (1ll << (price % BIT));
            break;
        } else pos -= ob[price].size();
    }
}
inline void op2(int size) {
    int i;
    for (i = 0; ; i++) {
        if (ob_bu1[i]) break;
    }
    for (i *= BUCKET; ; i++) {
        if (ob_bu2[i]) break;
    }
    while (size > 0) {
        while (!ob_bit[i]) i++;

        int j = __builtin_ctzll(ob_bit[i]);

        int price = i * BIT + j;

        while (ob[price].size() && size > 0) {
            u_int16_t &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            if (x == 0) {
                ob[price].pop();
                ob_bu1[price / (BUCKET * BIT)]--;
                ob_bu2[price / BIT]--;
                if (!ob[price].size()) ob_bit[price / BIT] ^= (1ll << (price % BIT));
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
    for (i *= BUCKET; ; i++) {
        if (ob_bu2[i]) break;
    }
    while (size > 0) {
        while (!ob_bit[i]) i++;

        int j = __builtin_ctzll(ob_bit[i]);

        int price = i * BIT + j;

        while (ob[price].size() && size > 0) {
            u_int16_t &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            ret += mn * price;
            if (x == 0) {
                ob[price].pop();
                ob_bu1[price / (BUCKET * BIT)]--;
                ob_bu2[price / BIT]--;
                if (!ob[price].size()) ob_bit[price / BIT] ^= (1ll << (price % BIT));
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
