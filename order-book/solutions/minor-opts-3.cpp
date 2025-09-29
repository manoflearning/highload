#include <array>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <immintrin.h>

// ABOUT UPDATES

const int N_UPDATES = 1'000'000;

struct Update { int8_t op; u_int32_t x; u_int16_t y; int8_t _paddr; }; // y is only for '+'

alignas(64) Update upds[N_UPDATES];
size_t upds_idx;

int parse_int(const char *&p) {
    // int len = std::min((const char *)memchr(p, ' ', e - p) - p, (const char *)memchr(p, '\n', e - p) - p);

    // if (len == 1) {
    //     return *p++ - '0';
    // } else if (len == 2) {
    //     int ret = 10 * p[0] + p[1] - 11 * '0';
    //     p += 2;
    //     return ret;
    // } else if (len == 3) {
    //     int ret = 100 * p[0] + 10 * p[1] + p[2] - 111 * '0';
    //     p += 3;
    //     return ret;
    // } else if (len == 4) {
    //     int ret = 1000 * p[0] + 100 * p[1] + 10 * p[2] + p[3] - 1111 * '0';
    //     p += 4;
    //     return ret;
    // } else {
    //     int ret = 10000 * p[0] + 1000 * p[1] + 100 * p[2] + 10 * p[3] + p[4] - 11111 * '0';
    //     p += 5;
    //     return ret;
    // }

    int ret = *p++ - '0';
    while ((unsigned)(*p - '0') <= 9) {
        ret = (ret << 1) + (ret << 3) + *p++ - '0';
    }
    return ret;
}

void prefault(void *ptr, off_t size) {
    volatile char *touch = (volatile char *)ptr;
    for (off_t i = 0; i < size; i += 4096) {
        touch[i];
    }
}

void input() {
    off_t size = lseek(0, 0, SEEK_END);
    void *ptr = mmap(0, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, 0, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    prefault(ptr, size);

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

const u_int16_t BUCKET_SZ1 = 16;
const u_int16_t BUCKET_SZ2 = BUCKET_SZ1 * BUCKET_SZ1;
const u_int16_t MAX_PRICE  = BUCKET_SZ1 * BUCKET_SZ1 * BUCKET_SZ1;
const u_int16_t QUEUE_CAP  = 256;

class queue {
    alignas(64) std::array<u_int16_t, QUEUE_CAP> arr{};
    u_int8_t st{}, en{};
public:
    u_int8_t siz{};
    void push(int x) { arr[en++] = x, siz++; }
    void pop() { st++, siz--; }
    u_int16_t &front() { return arr[st]; }
    // u_int8_t size() { return siz; }
    void erase(u_int8_t pos) {
        if ((int)pos * 2 <= siz) {
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
alignas(64) std::array<u_int16_t, BUCKET_SZ1> ob_bu1;
alignas(64) std::array<u_int16_t, BUCKET_SZ2> ob_bu2;
const __m256i zero = _mm256_set1_epi16(0);

// ABOUT SOLVE

void op0(int price, int size) {
    ob[price].push(size);
    ob_bu1[price / BUCKET_SZ2]++;
    ob_bu2[price / BUCKET_SZ1]++;
}
void op1(int pos) {
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
        if (pos < ob[price].siz) {
            ob[price].erase(pos);
            ob_bu1[price / BUCKET_SZ2]--;
            ob_bu2[price / BUCKET_SZ1]--;
            break;
        } else pos -= ob[price].siz;
    }
}
void op2(int size) {
    int i;
    {
        __m256i bu1 = _mm256_loadu_si256((const __m256i_u *)&ob_bu1[0]);
        __m256i bit = _mm256_cmpeq_epi16(bu1, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i = __builtin_ctz(~mask) / 2;
    }
    {
        i *= BUCKET_SZ1;
        __m256i bu2 = _mm256_loadu_si256((const __m256i_u *)&ob_bu2[i]);
        __m256i bit = _mm256_cmpeq_epi16(bu2, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i += __builtin_ctz(~mask) / 2;
    }

    for (int price = i * BUCKET_SZ1; size > 0; price++) {
        while (ob[price].siz && size > 0) {
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
int op3(int size) {
    int ret = 0;

    int i;
    {
        __m256i bu1 = _mm256_loadu_si256((const __m256i_u *)&ob_bu1[0]);
        __m256i bit = _mm256_cmpeq_epi16(bu1, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i = __builtin_ctz(~mask) / 2;
    }
    {
        i *= BUCKET_SZ1;
        __m256i bu2 = _mm256_loadu_si256((const __m256i_u *)&ob_bu2[i]);
        __m256i bit = _mm256_cmpeq_epi16(bu2, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i += __builtin_ctz(~mask) / 2;
    }

    for (int price = i * BUCKET_SZ1; size > 0; price++) {
        while (ob[price].siz && size > 0) {
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
