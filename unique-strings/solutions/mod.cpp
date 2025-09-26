#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

struct cfg {
    inline static const char CHARS[]    = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@#%*";
    static const u_int64_t CHARS_CNT    = 66;
    static const u_int64_t D            = CHARS_CNT + 1;
    static const u_int64_t TKN_MXLEN    = 16;
    static const u_int64_t MOD1         = 5050505;
    static const u_int64_t UB           = 18'446'744'073'709'551'615ULL / cfg::D - cfg::CHARS_CNT;
};

int tkn_hash[256];

void preprocessing() {
    for (int i = 0; i < cfg::CHARS_CNT; i++) {
        tkn_hash[cfg::CHARS[i]] = i + 1;
    }
}

class hash_set {
    size_t siz = 0;
    std::vector<std::vector<u_int64_t>> htb;
public:
    hash_set() {
        htb.resize(cfg::MOD1);
        htb.reserve(cfg::MOD1);
    }
    std::pair<u_int64_t, u_int64_t> rolling_hash(const std::string &s) {
        u_int64_t ret1 = 0, ret2 = 0;
        for (auto &c : s) {
            ret1 = cfg::D * ret1 + tkn_hash[c];
            ret2 = cfg::D * ret2 + tkn_hash[c];
            if (ret1 >= cfg::UB) ret1 %= cfg::MOD1;
        }
        return {ret1 % cfg::MOD1, ret2};
    }
    void insert(const std::string &s) {
        auto [hv1, hv2] = rolling_hash(s);
        for (auto &i : htb[hv1]) {
            if (i == hv2) return;
        }
        siz++;
        htb[hv1].push_back(hv2);
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
    
    std::cout << hst.size();
}
