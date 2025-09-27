#include <iostream>
#include <string>
#include <array>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <tuple>
#include <cstring>
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
    _mm256_set1_epi8('9'),
};

inline void update(__m256i &bit, char *&acc, const char *&s) {
    unsigned int mask = _mm256_movemask_epi8(bit);
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
            __m256i mx = _mm256_max_epu8(chunk, KEEP2[0]);
            __m256i mn = _mm256_min_epu8(chunk, KEEP2[1]);
            __m256i eq0 = _mm256_cmpeq_epi8(chunk, mx);
            __m256i eq1 = _mm256_cmpeq_epi8(chunk, mn);

            bit = _mm256_or_si256(bit, _mm256_and_si256(eq0, eq1));
        }

        update(bit, acc, s);

        s += 32;
    }

    while (s < e) {
        char c = *s++;
        if (c == '{' || c == '}' || (unsigned)(c - '0') <= 9 || c == 'd' || c == 't' || c == 'S') *acc++ = c;
    }

    *acc++ = ' ';
    *acc++ = '\0';

    return acc - *in;
}

void solve(const char *in, int n, u_int64_t &ans) {
    int stk = 0;
    
    std::array<std::tuple<int, int>, 10> transactions;
    int user_id = 0;
    bool is_usd = 0;
    int idx = 0;

    int to_user_id = 0, amount = 0;
    int cnt_t = 0; // cnt_t == 3 iff canceled == true
    
    for (int i = 1; i + 1 < n; i++) {
        char c = in[i], cp1 = in[i - 1];
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
                if (cnt_t < 3) {
                    transactions[idx++] = {to_user_id, amount};
                }
                to_user_id = amount = cnt_t = 0;
            }
        } else if ((unsigned)(c - '0') <= 9) {
            int tmp = in[i] - '0';
            for (i++; (unsigned)(in[i] - '0') <= 9; i++) {
                tmp = 10 * tmp + in[i] - '0';
            }
            i--;

            if (stk == 1) {
                user_id = tmp;
            } else {
                cp1 == 'd' ? to_user_id = tmp : amount = tmp;
            }
        } else if (stk == 2 && c == 't') {
            cnt_t++;
        } else if (c == 'S') {
            is_usd = 1;
        }
    }
}

int main() {
    char *in;
    size_t n = input(&in);

    u_int64_t ans = 0;
    solve(in, n, ans);

    std::cout << ans << '\n';
}
