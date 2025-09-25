#include <cstdio>
#include <cstddef>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    struct stat st{};
    fstat(STDIN_FILENO, &st);
    const size_t size = st.st_size;

    void *buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, STDIN_FILENO, 0);

    madvise(buf, size, MADV_SEQUENTIAL);
    madvise(buf, size, MADV_WILLNEED);
    madvise(buf, size, MADV_HUGEPAGE);

    const char *s = (const char *) buf;
    const char *e = s + size;

    u_int64_t sum = 0;
    u_int64_t x = 0;
    while (s < e) {
        while ('0' <= *s && *s <= '9') {
            x = 10 * x + *s - '0';
            s++;
        }
        while (*s == '\n') {
            sum += x;
            x = 0;
            s++;
        }
    }

    printf("%lu", sum);
}
