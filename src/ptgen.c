#include "inttypes.h"

// libc
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

static void error(const char *fmt, ...) {
    fprintf(stderr, "Error: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static void die(const char *fmt, ...) {
    fprintf(stderr, "Fatal: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

#include "lexer.c"

i32 main(i32 argc, u8 **argv) {
    const u8 *filepath = argv[1];

    FILE *fd = fopen(filepath, "r");
    if (!fd) {
        die("(fopen) %s\n", strerror(errno));
    }

    if(fseek(fd, 0, SEEK_END) == -1) {
        die("(fseek) %s\n", strerror(errno));
    }

    i64 size = ftell(fd);
    if (size == -1) {
        die("(ftell) %s\n", strerror(errno));
    }

    if (fseek(fd, 0, SEEK_SET) == -1) {
        die("(fseek) %s\n", strerror(errno));
    }

    u8 buf[size+1];
    buf[size] = 0;

    if (fread(buf, 1, size, fd) < size) {
        die("(fread) failed to read entire file!\n");
    }

    lex(filepath, buf, size);

    fclose(fd);
    return 0;
}
