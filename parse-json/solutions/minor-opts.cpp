#include <iostream>
#include <array>
#include <sys/mman.h>
#include <unistd.h>
#include <immintrin.h>

const __m256i KEEP1[] = {
    _mm256_set1_epi8('{'),
    _mm256_set1_epi8('}'),
    _mm256_set1_epi8('d'),
    _mm256_set1_epi8('t'),
    _mm256_set1_epi8('S'),
};
const __m256i KEEP2[] {
    _mm256_set1_epi8('0'),
    _mm256_set1_epi8((char)9),
};

inline void update(__m256i &bit, char *&acc, const char *&s) {
    unsigned int mask = _mm256_movemask_epi8(bit);
    // avg __builtin_popcount(mask)             = 7.1
    // P(ith bit of mask is 1 | 0 <= i < 32)    = 0.22
    while (mask) {
        int idx = __builtin_ctz(mask);
        *acc++ = s[idx];
        mask ^= (1 << idx);
    }
}

size_t input(char **in) {
    *in = new char[(int)(92085065 * 1.1)];
    
    off_t size = lseek(0, 0, SEEK_END);
    void *ptr = mmap(0, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, 0, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    const char *s = (const char *) ptr;
    const char *e = s + size;

    char *acc = *in;

    *acc++ = ' ';
    while (s + 31 < e) {
        __m256i chunk = _mm256_loadu_si256((__m256i *)s);
        
        __m256i bit;
        {
            __m256i res0 = _mm256_cmpeq_epi8(KEEP1[0], chunk);
            __m256i res1 = _mm256_cmpeq_epi8(KEEP1[1], chunk);
            __m256i res2 = _mm256_cmpeq_epi8(KEEP1[2], chunk);
            __m256i res3 = _mm256_cmpeq_epi8(KEEP1[3], chunk);
            __m256i res4 = _mm256_cmpeq_epi8(KEEP1[4], chunk);

            __m256i bit0 = _mm256_or_si256(res0, res1);
            __m256i bit1 = _mm256_or_si256(res2, res3);

            bit = _mm256_or_si256(_mm256_or_si256(bit0, bit1), res4);
        }
        {
            // Use 4 SIMD ops instead of 6,
            // using ((unsigned)(c - '0') <= 9 iff '0' <= c && c <= '9')
            // But is it REALLY faster??

            __m256i sb = _mm256_sub_epi8(chunk, KEEP2[0]);
            __m256i mn = _mm256_min_epu8(sb, KEEP2[1]);
            __m256i eq = _mm256_cmpeq_epi8(sb, mn);
            bit = _mm256_or_si256(bit, eq);

            // 6 SIMD ops equals to '0' <= c && c <= '9':
            // __m256i mx = _mm256_max_epu8(chunk, KEEP2[0]);
            // __m256i mn = _mm256_min_epu8(chunk, KEEP2[1]); // this should be '9', not 9
            // __m256i eq0 = _mm256_cmpeq_epi8(chunk, mx);
            // __m256i eq1 = _mm256_cmpeq_epi8(chunk, mn);
            // bit = _mm256_or_si256(bit, _mm256_and_si256(eq0, eq1));
        }

        update(bit, acc, s); // TODO: replace it with shuffle

        s += 32;
    }

    while (s < e) {
        char c = *s++;

        // In uniform random input data:
        // P((unsigned)(c - '0') <= 9)  = 51.8%
        // P(c == 't')                  = 18.7%
        // P(c == 'd')                  = 12.8%
        // P(c == '{') = P(c == '}')    =  8.2%
        // P(c == 'S')                  =  0.3%
        if ((unsigned)(c - '0') <= 9 || c == 't' || c == 'd' || c == '{' || c == '}' || c == 'S') *acc++ = c;
    }

    *acc++ = '\0';

    return acc - *in;
}

// For profiling purpose
inline void case1(int &stk) { stk++; }
inline void case2(int &stk, bool &is_usd, int &idx, u_int64_t &ans, std::array<std::pair<int, int>, 10> &transactions, int &user_id, int &cnt_t, int &to_user_id, int &amount) {
    stk--;
    if (stk == 1) { [[likely]] // 83%
        if (cnt_t < 3) {
            transactions[idx++] = {to_user_id, amount};
        }
        to_user_id = amount = cnt_t = 0;
    } else {
        if (is_usd) { // [[unlikely]] // 20%
            for (int j = 0; j < idx; j++) {
                ans += (transactions[j].first != user_id) * transactions[j].second;
            }
        }
        user_id = is_usd = idx = 0;
    }
}
inline void case3(int &i, const char *&in, const int &stk, int &user_id, int &to_user_id, int &amount, const char &cp1) {
    int tmp = in[i] - '0';
    for (i++; (unsigned)(in[i] - '0') <= 9; i++) {
        tmp = 10 * tmp + in[i] - '0';
    }
    i--;

    int &assigned = (stk == 1 ? user_id : (cp1 == 'd' ? to_user_id : amount)); // To avoid if-else
    assigned = tmp;
}
inline void case4(int &cnt_t) { cnt_t++; }

void solve(const char *in, int n, u_int64_t &ans) {
    std::array<std::pair<int, int>, 10> transactions; // array of pair to improve cache hit

    int stk = 0;

    int user_id = 0;
    bool is_usd = 0;
    int idx = 0;

    int to_user_id = 0, amount = 0;
    int cnt_t = 0; // cnt_t == 3 iff canceled == true
    
    for (int i = 1; i < n; i++) {
        char c = in[i], cp1 = in[i - 1];

        if (c == '{') case1(stk);
        else if (c == '}') case2(stk, is_usd, idx, ans, transactions, user_id, cnt_t, to_user_id, amount);
        else if ((unsigned)(c - '0') <= 9) case3(i, in, stk, user_id, to_user_id, amount, cp1);
        else if (stk == 2 && c == 't') case4(cnt_t);

        is_usd |= (c == 'S'); // Replace else if (c == 'S') { is_usd = 1; }
    }
}

int main() {
    char *in;
    size_t n = input(&in);

    u_int64_t ans = 0;
    solve(in, n, ans);

    std::cout << ans << '\n';
}
