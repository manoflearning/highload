#include <array>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <vector>

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

const int MAX_PRICE = 3500;
const int QUEUE_CAP = 256;

class queue {
    std::array<int, QUEUE_CAP> arr{};
    u_int8_t st{}, en{};
public:
    void push(int x) { arr[en++] = x; }
    void pop() { st++; }
    int &front() { return arr[st]; }
    u_int8_t size() { return en - st; }
    void erase(u_int8_t pos) {
        if ((int)pos * 2 <= size()) {
            for (u_int8_t i = st + pos; i != st; i--) {
                arr[i] = arr[(u_int8_t)(i - 1)];
            }
            st++;
        } else {
            for (u_int8_t i = st + pos + 1; i != en; i++) {
                arr[(u_int8_t)(i - 1)] = arr[i];
            }
            en--;
        }
    }
};

alignas(32) std::array<queue, MAX_PRICE> ob;

struct Fenwick { // 0-indexed
    int flag{4096}, cnt{12}; // array size
    std::array<int, 4096> arr{}, t{};
    void add(int p, int value) { // add value at position p
        arr[p] += value;
        while (p < flag) {
            t[p] += value;
            p |= p + 1;
        }
    }
    int query(int x) {
        int ret = 0;
        while (x >= 0) ret += t[x], x = (x & (x + 1)) - 1;
        return ret;
    }
    int kth(int k) { // find the kth smallest number (1-indexed)
        int l = 0, r = arr.size();
        for (int i = 0; i <= cnt; i++) {
            int mid = (l + r) >> 1;
            int val = mid ? t[mid - 1] : t.back();
            if (val >= k) r = mid;
            else l = mid, k -= val;
        }
        return l;
    }
} fw;

// ABOUT SOLVE

inline void op0(int price, int size) {
    ob[price].push(size);
    fw.add(price, 1);
}
inline void op1(int pos) {
    int k = fw.kth(pos + 1);
    ob[k].erase(pos - fw.query(k - 1));
    fw.add(k, -1);
}
inline void op2(int size) {
    for (int price = fw.kth(1); size > 0; price++) {
        while (ob[price].size() && size > 0) {
            int &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            if (x == 0) { [[unlikely]]
                ob[price].pop();
                fw.add(price, -1);
            }
        }
    }
}
inline int op3(int size) {
    int ret = 0;

    for (int price = fw.kth(1); size > 0; price++) {
        while (ob[price].size() && size > 0) {
            int &x = ob[price].front();

            int mn = (x < size ? x : size);
            x -= mn, size -= mn;
            ret += mn * price;
            if (x == 0) { [[unlikely]]
                ob[price].pop();
                fw.add(price, -1);
            }
        }
    }

    return ret;
}

void solve() {
    for (int i = 0; i < upds_idx; i++) {
        auto &it = upds[i];
        if (it.op == 0) {
            op0(it.x, it.y);
        } else if (it.op == 1) {
            op1(it.x);
        } else { 
            op2(it.x);
        }
    }
}

int main() {
    input();

    solve();

    std::cout << op3(1000) << '\n';
}
