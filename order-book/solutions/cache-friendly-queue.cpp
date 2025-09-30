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

#define N_UPDATES 1'000'000

struct Update {
    int32_t x;
    int16_t y;
    int8_t op;
};

alignas(2 << 20) Update upds[N_UPDATES];

int parse_int(const char *&p) {
    int ret = *p++ - '0';
    while ((unsigned)(*p - '0') <= 9) {
        ret = (ret << 1) + (ret << 3) + *p++ - '0';
    }
    return ret;
}

void prefault(const void *ptr, const off_t size) {
    volatile char *touch = (volatile char *)ptr;
    for (off_t i = 0; i < size; i += 4096) {
        touch[i];
    }
}

void input() {
    off_t size = lseek(0, 0, SEEK_END);
    void *ptr = mmap(0, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE | (21 << MAP_HUGE_SHIFT), 0, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);
    madvise(ptr, size, MADV_POPULATE_WRITE);

    madvise(upds, sizeof(upds), MADV_SEQUENTIAL);
    madvise(upds, sizeof(upds), MADV_WILLNEED);
    madvise(upds, sizeof(upds), MADV_HUGEPAGE);
    madvise(upds, sizeof(upds), MADV_POPULATE_WRITE);

    prefault(ptr, size);

    const char *s = (const char *)ptr;
    const char *e = s + size;
    
    for (int i = 0; i < N_UPDATES; i++) {
        const char *line = (const char *)memchr(s, '\n', e - s);

        char c = *s;
        if (c == '+') {
            upds[i].op = 0;
            s += 2;
            upds[i].x = parse_int(s);
            s += 1;
            upds[i].y = parse_int(s);
        } else {
            upds[i].op = (c == '-' ? 1 : 2);
            s += 2;
            upds[i].x = parse_int(s);
        }

        s = line + 1;
    }
}

// ABOUT ORDER BOOK

#define BS1         4
#define BS2         8
#define BUCKET_SZ1  16
#define BUCKET_SZ2  256
#define MAX_PRICE   4096
#define QUEUE_CAP   256

struct queue_meta { uint32_t sum{}; uint_fast8_t siz{}, st{}, en{}; };
struct queue {
    std::array<uint16_t, QUEUE_CAP> arr{};

    void erase(uint_fast8_t pos, queue_meta& qm) {
        if ((int)pos * 2 <= qm.siz) {
            qm.sum -= arr[qm.st + pos];
            for (uint_fast8_t i = qm.st + pos; i != qm.st; i--) {
                arr[i] = arr[(uint_fast8_t)(i - 1)];
            }
            qm.st++, qm.siz--;
        } else {
            qm.sum -= arr[qm.st + pos];
            for (uint_fast8_t i = qm.st + pos + 1; i != qm.en; i++) {
                arr[(uint_fast8_t)(i - 1)] = arr[i];
            }
            qm.en--, qm.siz--;
        }
    }
};

alignas(64) std::array<queue_meta, MAX_PRICE> ob_meta;
alignas(64) std::array<queue, MAX_PRICE> ob_qu;

alignas(64) std::array<uint16_t, BUCKET_SZ1> ob_bu1;
alignas(64) std::array<uint16_t, BUCKET_SZ2> ob_bu2;
const __m256i zero = _mm256_set1_epi16(0);

// ABOUT SOLVE

void op0(const int price, const int size) {
    ob_meta[price].sum += size;
    ob_meta[price].siz++;

    ob_qu[price].arr[ob_meta[price].en++] = size;

    ob_bu1[price >> BS2]++;
    ob_bu2[price >> BS1]++;
}
void op1(uint16_t pos) {
    int i;
    for (i = 0; ; i++) {
        if (pos < ob_bu1[i]) break;
        else pos -= ob_bu1[i];
    }
    for (i <<= BS1; ; i++) {
        if (pos < ob_bu2[i]) break;
        else pos -= ob_bu2[i];
    }

    int price;
    for (price = (i << BS1); pos >= ob_meta[price].siz; price++) {
        pos -= ob_meta[price].siz;
    }

    ob_qu[price].erase(pos, ob_meta[price]);
    ob_bu1[price >> BS2]--;
    ob_bu2[price >> BS1]--;
}
void op2(int size) {
    int i;
    {
        __m256i bu1 = _mm256_loadu_si256((const __m256i_u *)&ob_bu1[0]);
        __m256i bit = _mm256_cmpeq_epi16(bu1, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i = __builtin_ctz(~mask) >> 1;
    }
    {
        i <<= BS1;
        __m256i bu2 = _mm256_loadu_si256((const __m256i_u *)&ob_bu2[i]);
        __m256i bit = _mm256_cmpeq_epi16(bu2, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i += __builtin_ctz(~mask) >> 1;
    }

    int price;
    for (price = (i << BS1); ob_meta[price].sum <= size; price++) {
        if (!ob_meta[price].sum) continue;

        size -= ob_meta[price].sum;
        ob_bu1[price >> BS2] -= ob_meta[price].siz;
        ob_bu2[price >> BS1] -= ob_meta[price].siz;

        ob_meta[price].sum = ob_meta[price].siz = ob_meta[price].st = ob_meta[price].en = 0;
    }

    while (size > 0) {
        uint16_t &x = ob_qu[price].arr[ob_meta[price].st];

        int mn = (x < size ? x : size);
        x -= mn, size -= mn, ob_meta[price].sum -= mn;
        if (x == 0) {
            ob_meta[price].st++, ob_meta[price].siz--;
            ob_bu1[price >> BS2]--;
            ob_bu2[price >> BS1]--;
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
        i = __builtin_ctz(~mask) >> 1;
    }
    {
        i <<= BS1;
        __m256i bu2 = _mm256_loadu_si256((const __m256i_u *)&ob_bu2[i]);
        __m256i bit = _mm256_cmpeq_epi16(bu2, zero);
        unsigned int mask = _mm256_movemask_epi8(bit);
        i += __builtin_ctz(~mask) >> 1;
    }

    int price;
    for (price = (i << BS1); ob_meta[price].sum <= size; price++) {
        if (!ob_meta[price].sum) continue;

        size -= ob_meta[price].sum;
        ob_bu1[price >> BS2] -= ob_meta[price].siz;
        ob_bu2[price >> BS1] -= ob_meta[price].siz;
        ret += ob_meta[price].sum * price;

        ob_meta[price].sum = ob_meta[price].siz = ob_meta[price].st = ob_meta[price].en = 0;
    }

    while (size > 0) {
        uint16_t &x = ob_qu[price].arr[ob_meta[price].st];

        int mn = (x < size ? x : size);
        x -= mn, size -= mn, ob_meta[price].sum -= mn;
        ret += mn * price;
        if (x == 0) {
            ob_meta[price].st++, ob_meta[price].siz--;
            ob_bu1[price >> BS2]--;
            ob_bu2[price >> BS1]--;
        }
    }

    return ret;
}

void solve() {
    for (auto &it : upds) {
        // auto &it = upds[i];
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
