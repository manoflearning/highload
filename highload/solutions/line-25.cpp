#include <iostream>
#include <cstddef>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    struct stat st{};
    fstat(STDIN_FILENO, &st);
    const size_t size = st.st_size;

    void *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, STDIN_FILENO, 0);

    madvise(ptr, size, MADV_SEQUENTIAL);
    madvise(ptr, size, MADV_WILLNEED);
    madvise(ptr, size, MADV_HUGEPAGE);

    const unsigned char *s = (const unsigned char *) ptr;
    const unsigned char *e = s + size;

    u_int64_t sum = 0;
    u_int64_t x = 0;
    while (s < e) {
        while ('0' <= *s) {
            x = 10 * x + *s - '0';
            s++;
        }
        while (*s == '\n') {
            sum += x;
            x = 0;
            s++;
        }
    }

    std::cout << sum;
}
