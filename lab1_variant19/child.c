#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdbool.h>

static bool is_vowel(char c) {
    char lower = (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
    return (lower == 'a' || lower == 'e' || lower == 'i' ||
            lower == 'o' || lower == 'u' || lower == 'y');
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        const char msg[] = "error: no output filename provided\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        const char msg[] = "error: failed to open output file\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    }

    char in_buf[4096];
    char out_buf[4096];
    ssize_t bytes;

    while ((bytes = read(0, in_buf, sizeof(in_buf))) > 0) {
        int out_len = 0;
        for (ssize_t i = 0; i < bytes; ++i) {
            if (!is_vowel(in_buf[i])) {
                out_buf[out_len++] = in_buf[i];
            }
        }
        if (out_len > 0) {
            write(file, out_buf, out_len);
        }
    }

    close(file);
    return 0;
}
