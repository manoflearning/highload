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
#include <cstdint>

// ABOUT UPDATES

const int N_UPDATES = 1'000'000;

struct Update {
    int32_t x;
    int16_t y;
    int8_t op;
};

alignas(64) Update upds[N_UPDATES];
size_t upds_idx;

int parse_int(const char *&p) {
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
    }

    assert(upds_idx == N_UPDATES);
}

// ABOUT ORDER BOOK

const uint16_t BUCKET_SZ1 = 16;
const uint16_t BUCKET_SZ2 = BUCKET_SZ1 * BUCKET_SZ1;
const uint16_t MAX_PRICE  = BUCKET_SZ1 * BUCKET_SZ1 * BUCKET_SZ1;
const uint16_t QUEUE_CAP  = 256;

class queue {
    alignas(64) std::array<uint16_t, QUEUE_CAP> arr{};
    uint_fast8_t st{}, en{};
public:
    uint_fast8_t siz{};
    void push(int x) { arr[en++] = x, siz++; }
    void pop() { st++, siz--; }
    uint16_t &front() { return arr[st]; }
    void erase(uint_fast8_t pos) {
        if ((int)pos * 2 <= siz) {
            for (uint_fast8_t i = st + pos; i != st; i--) {
                arr[i] = arr[(uint_fast8_t)(i - 1)];
            }
            st++, siz--;
        } else {
            for (uint_fast8_t i = st + pos + 1; i != en; i++) {
                arr[(uint_fast8_t)(i - 1)] = arr[i];
            }
            en--, siz--;
        }
    }
};

std::array<queue, MAX_PRICE> ob;
alignas(64) std::array<uint16_t, BUCKET_SZ1> ob_bu1;
alignas(64) std::array<uint16_t, BUCKET_SZ2> ob_bu2;
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
            uint16_t &x = ob[price].front();

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
            uint16_t &x = ob[price].front();

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
