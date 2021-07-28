#include "inttypes.h"

// libc
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#define CBEGIN "\033["
#define CEND   "m"

#define FG_BLACK      "30"
#define BG_BLACK      "40"
#define FG_RED        "31"
#define BG_RED        "41"
#define FG_GREEN      "32"
#define BG_GREEN      "42"
#define FG_YELLOW     "33"
#define BG_YELLOW     "43"
#define FG_BLUE       "34"
#define BG_BLUE       "44"
#define FG_MAGENTA    "35"
#define BG_MAGENTA    "45"
#define FG_CYAN       "36"
#define BG_CYAN       "46"
#define FG_WHITE      "37"
#define BG_WHITE      "47"
#define RESET         "0"
#define BOLD_ON       "1"
#define UNDERLINE_ON  "4"
#define INVERSE_ON    "7"
#define BOLD_OFF      "21"
#define UNDERLINE_OFF "24"
#define INVERSE_OFF   "27"

static void error(const char *fmt, ...) {
    fprintf(stderr, CBEGIN FG_RED CEND);
    fprintf(stderr, "Error: ");
    fprintf(stderr, CBEGIN RESET CEND);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static void die(const char *fmt, ...) {
    fprintf(stderr, CBEGIN FG_RED ";" BOLD_ON CEND);
    fprintf(stderr, "Fatal: ");
    fprintf(stderr, CBEGIN RESET CEND);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

#include "lexer.c"
#include "parser.c"

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
