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

  abort();
}

static void inline xassert(bool cond, const char *fmt, ...) {
  if (!cond) {
    va_list args;
    va_start(args, fmt);
    die(fmt, args);
    va_end(args);
  }
}

static const char *xstrndup(const char *src, u8 len) {
  const char *ret = strndup(src, len);
  if (!ret) {
    die("strndup failed - %s\n", strerror(errno));
  }
  return ret;
};

#include "lexer.c"
#include "ast.c"
#include "parser.c"

i32 main(i32 argc, u8 **argv) {
  if (argc != 2) {
    die("Usage: ptgen input_file");
  }

  const u8 *filepath = argv[1];

  /* Open and read entire file into buffer */

  FILE *fd = fopen(filepath, "r");
  xassert(fd, "(fopen) %s\n", strerror(errno));

  xassert(fseek(fd, 0, SEEK_END) != -1, "(fseek) %s\n", strerror(errno));

  i64 size = ftell(fd);
  xassert(size != -1, "(ftell) %s\n", strerror(errno));

  xassert(fseek(fd, 0, SEEK_SET) != -1, "(fseek) %s\n", strerror(errno));

  u8 buf[size+1];
  buf[size] = 0;

  xassert(fread(buf, 1, size, fd) == size, "(fread) failed to read entire file!\n");

  struct token_buffer tok_buf;
  lex(&tok_buf, filepath, buf, size);
  dump_token_buffer(&tok_buf);

  struct ast_node *root = parse(&tok_buf);
  dump_ast_to_dot(root, "ast.dot");
  dump_ast_to_tex(root, "ast.tex");

  fclose(fd);
  return 0;
}
