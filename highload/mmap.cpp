#include <cstdio>
#include <cstddef>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    struct stat st{};
    fstat(STDIN_FILENO, &st);
    size_t size = st.st_size;

    const char *buf = (const char *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, STDIN_FILENO, 0);

    const char *s = buf;
    const char *e = s + size;

    u_int64_t sum = 0;
    u_int64_t x = 0;
    while (s < e) {
        if (0 <= *s - '0' && *s - '0' <= 9) {
            x = 10 * x + (*s - '0');
        } else {
            sum += x;
            x = 0;
        }
        s++;
    }

    printf("%lu", sum);
}
