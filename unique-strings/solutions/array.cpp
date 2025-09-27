#include <iostream>
#include <vector>
#include <array>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>

struct cfg {
    inline static const char CHARS[]    = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@#%*";
    static const u_int64_t CHARS_CNT    = 66;
    static const u_int64_t D            = CHARS_CNT + 1;
    static const u_int64_t TKN_MXLEN    = 16;
    static const u_int64_t MOD1         = 8388608; // 2^23
    static const size_t ARRAY_SZ        = 5;
};

int tkn_hash[256];

void preprocessing() {
    for (int i = 0; i < cfg::CHARS_CNT; i++) {
        tkn_hash[cfg::CHARS[i]] = i + 1;
    }
}

class hash_set {
    size_t siz = 0;
    std::vector<std::array<u_int64_t, cfg::ARRAY_SZ>> htb;
    std::vector<std::array<bool, cfg::ARRAY_SZ>> mask;
public:
    hash_set() {
        htb.resize(cfg::MOD1);
        mask.resize(cfg::MOD1, {0, 0, 0, 0, 0});
    }
    std::pair<u_int64_t, u_int64_t> rolling_hash(const std::string &s) {
        u_int64_t ret1 = 0, ret2 = 0;
        for (auto &c : s) {
            ret1 = cfg::D * ret1 + tkn_hash[c];
            ret2 = cfg::D * ret2 + tkn_hash[c];
        }
        return {ret1 & (cfg::MOD1 - 1), ret2};
    }
    void insert(const std::string &s) {
        auto [hv1, hv2] = rolling_hash(s);
        for (int i = 0; i < cfg::ARRAY_SZ; i++) {
            if (!mask[hv1][i]) {
                htb[hv1][i] = hv2;
                mask[hv1][i] = 1;
                siz++;
                return;
            } else {
                if (htb[hv1][i] == hv2) return;
            }
        }
    }
    size_t size() { return siz; }
};

int main() {
    struct stat st{};
    fstat(STDIN_FILENO, &st);
    size_t size = st.st_size;
    
    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, STDIN_FILENO, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    preprocessing();

    const char *s = (const char *) ptr;
    const char *e = s + size;

    hash_set hst;
    std::string tkn;
    while (s < e) {
        while (*s != '\n') {
            tkn += *s;
            s++;
        }

        hst.insert(tkn);
        tkn.clear();
        s++;
    }
    
    std::cout << hst.size() << '\n';
}
