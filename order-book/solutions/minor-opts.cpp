#include <array>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cassert>

// ABOUT UPDATES

const int N_UPDATES = 1'000'000;

struct Update {
    int op, x, y; // y is only for '+'
};

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

const int MAX_PRICE = 4900; // 70^2
const int SQ        = 70;
const int N_BUCKET  = MAX_PRICE / SQ;

class queue {
    int *arr;
    int st, en, cap; // [st, en)
public:
    queue() {
        cap = 64;
        arr = new int[cap];
        st = en = 0;
    }
    ~queue() { delete[] arr; }

    void reallocate_() {
        int *tmp = new int[cap << 1];
        for (int i = 0, j = st; j != en; i++, j = add_mod_(j, 1)) {
            tmp[i] = arr[j];
        }

        en = sub_mod_(en, st);
        st = 0;
        cap <<= 1;

        delete[] arr;
        arr = tmp;
    }
    inline int add_mod_(int x, int y) {
        int ret = x + y;
        if (cap <= ret) ret -= cap;
        return ret;
    }
    inline int sub_mod_(int x, int y) {
        int ret = x - y;
        if (ret < 0) ret += cap;
        return ret;
    }
    void push(int x) {
        if (st == add_mod_(en, 1)) reallocate_();

        arr[en] = x;
        en = add_mod_(en, 1);
    }
    void pop() {
        assert(st != en);
        st = add_mod_(st, 1);
    }
    int &front() {
        return arr[st];
    }
    int size() {
        return sub_mod_(en, st);
    }
    void erase(int pos) {
        for (int i = add_mod_(st, pos + 1); i != en; i = add_mod_(i, 1)) {
            arr[sub_mod_(i, 1)] = arr[i];
        }
        en = sub_mod_(en, 1);
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
            int &x = ob[price].front();

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
            int &x = ob[price].front();

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
