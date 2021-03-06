struct location {
  const u8 *at;
  const u8 *file;
  u64 line;
  u8 len;
};

struct lexer {
  struct location loc;
};

enum token_type {
  COMMA = 0,
  /* Binary ops */
  SUB,
  ADD,
  DIV,
  MUL,
  POW,
  FACTORIAL,
  ASSIGN,
  /* Parens */
  LPAREN,
  RPAREN,
  LBRACKET,
  RBRACKET,
  LBRACE,
  RBRACE,
  /* Reserved */
  CREATE_OP,
  ANNIHI_OP,
  SUM,
  EXP,
  SQRT,
  /* Literals */
  NUMBER,
  IDENTIFIER,
  /* EOF, don't move */
  END_OF_FILE,
};

static const char *token_names[] = {
  [COMMA]       = "COMMA",
  [SUB]         = "SUB",
  [ADD]         = "ADD",
  [DIV]         = "DIV",
  [MUL]         = "MUL",
  [POW]         = "POW",
  [FACTORIAL]   = "FACTORIAL",
  [ASSIGN]      = "ASSIGN",
  [LPAREN]      = "LPAREN",
  [RPAREN]      = "RPAREN",
  [LBRACKET]    = "LBRACKET",
  [RBRACKET]    = "RBRACKET",
  [LBRACE]      = "LBRACE",
  [RBRACE]      = "RBRACE",
  [CREATE_OP]   = "CREATE_OP",
  [ANNIHI_OP]   = "ANNIHI_OP",
  [SUM]         = "SUM",
  [EXP]         = "EXP",
  [SQRT]        = "SQRT",
  [NUMBER]      = "NUMBER",
  [IDENTIFIER]  = "IDENTIFIER",
  [END_OF_FILE] = "END_OF_FILE",
};

struct token {
  enum token_type type;
  struct location loc;
};

#define TOKEN(t, l) \
  (struct token) { .type = t, .loc = l }

#define MAX_NUM_TOKENS 1024

struct token_buffer {
  struct token tokens[MAX_NUM_TOKENS];
  u16 num_tokens;
};

static void dump_token_buffer(struct token_buffer *tok_buf) {
  for (u16 i = 0; i < tok_buf->num_tokens; ++i) {
    puts(token_names[tok_buf->tokens[i].type]);
  }
}

void print_location(struct location *loc, const u8 *fmt, ...) {
  printf(CBEGIN FG_CYAN CEND);
  u64 size = printf("  %s:%u | ", loc->file, loc->line);
  printf(CBEGIN RESET CEND);

  const u8 *begin = loc->at;
  while (*begin && *begin != '\n') {
    begin--;
  }
  if (begin < loc->at) {
    begin++;
  }

  const u8 *end = begin;
  while (*(end) && *(end) != '\n') {
    putchar(*end);
    end++;
  }
  if (end > begin) {
    end--;
  }

  putchar('\n');

  for (u8 i = 0; i < size + (loc->at - begin); ++i) {
    putchar(' ');
  }

  printf(CBEGIN FG_CYAN CEND);

  putchar('^');

  if (loc->len > 3) {
    for (u8 i = 0; i < loc->len-2; ++i) {
      putchar('-');
    }
    putchar('^');
  }

  putchar(' ');
  putchar(' ');

  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  printf(CBEGIN RESET CEND);
}

static inline bool match_str(struct lexer *lex, const u8 *str) {
  u8 len = strlen(str);
  for (u8 i = 0; i < len; ++i) {
    if (lex->loc.at[i] != str[i]) {
      return false;
    }
  }

  return true;
}

static void consume_until(struct lexer *lex, const u8 *str) {
  while (*(lex->loc.at) && !match_str(lex, str)) {
    lex->loc.at++;
  }

  if (!*(lex->loc.at)) {
    die("(consume_until) runaway %s\n", str);
  }
}

static void next_token(struct lexer *lex, struct token *tok) {
  /* Consume whitespace and newlines */
  while (*lex->loc.at && (isspace(*lex->loc.at) || *lex->loc.at == '\n' || *lex->loc.at == '#')) {

    if (*lex->loc.at == '#') {
      consume_until(lex, "\n");
    }

    if (*lex->loc.at == '\n') {
      lex->loc.line++;
    }

    lex->loc.at++;
  }

  /* 1 char tokens */

  switch (*lex->loc.at) {
  case ',': {
    *tok = TOKEN(COMMA, lex->loc);
    lex->loc.at++;
    return;
  }
  case '-': {
    *tok = TOKEN(SUB, lex->loc);
    lex->loc.at++;
    return;
  }
  case '+': {
    *tok = TOKEN(ADD, lex->loc);
    lex->loc.at++;
    return;
  }
  case '/': {
    *tok = TOKEN(DIV, lex->loc);
    lex->loc.at++;
    return;
  }
  case '*': {
    *tok = TOKEN(MUL, lex->loc);
    lex->loc.at++;
    return;
  }
  case '^': {
    *tok = TOKEN(POW, lex->loc);
    lex->loc.at++;
    return;
  }
  case '!': {
    *tok = TOKEN(FACTORIAL, lex->loc);
    lex->loc.at++;
    return;
  }
  case '=': {
    *tok = TOKEN(ASSIGN, lex->loc);
    lex->loc.at++;
    return;
  }
  case '(': {
    *tok = TOKEN(LPAREN, lex->loc);
    lex->loc.at++;
    return;
  }
  case ')': {
    *tok = TOKEN(RPAREN, lex->loc);
    lex->loc.at++;
    return;
  }
  case '[': {
    *tok = TOKEN(LBRACKET, lex->loc);
    lex->loc.at++;
    return;
  }
  case ']': {
    *tok = TOKEN(RBRACKET, lex->loc);
    lex->loc.at++;
    return;
  }
  case '{': {
    *tok = TOKEN(LBRACE, lex->loc);
    lex->loc.at++;
    return;
  }
  case '}': {
    *tok = TOKEN(RBRACE, lex->loc);
    lex->loc.at++;
    return;
  }
  }

  /* Reserved */
  if (*lex->loc.at == 'c') {
    *tok = TOKEN(CREATE_OP, lex->loc);
    lex->loc.at++;
    return;
  } else if (*lex->loc.at == 'a') {
    *tok = TOKEN(ANNIHI_OP, lex->loc);
    lex->loc.at++;
    return;
  } else if (strncmp(lex->loc.at, "sum", 3) == 0) {
    *tok = TOKEN(SUM, lex->loc);
    lex->loc.at += 3;
    return;
  } else if (strncmp(lex->loc.at, "exp", 3) == 0) {
    *tok = TOKEN(EXP, lex->loc);
    lex->loc.at += 3;
    return;
  } else if (strncmp(lex->loc.at, "sqrt", 4) == 0) {
    *tok = TOKEN(SQRT, lex->loc);
    lex->loc.at += 4;
    return;
  }

  /* Numbers */
  if (isdigit(*lex->loc.at)) {
    struct location begin = lex->loc;
    while (isdigit(*lex->loc.at)) {
      lex->loc.at++;
    }
    *tok = TOKEN(NUMBER, begin);
    return;
  }

  /* Identifier */
  if (isalpha(*lex->loc.at)) {
    struct location begin = lex->loc;
    while (isalpha(*lex->loc.at)) {
      lex->loc.at++;
    }
    *tok = TOKEN(IDENTIFIER, begin);
    tok->loc.len = lex->loc.at - begin.at;
    return;
  }

  if (!*lex->loc.at) {
    *tok = TOKEN(END_OF_FILE, lex->loc);
    return;
  }

  print_location(&lex->loc, "Unknown token\n");
  die("Doesn't know how to handle unknown tokens\n");
}

static void lex(struct token_buffer *tok_buf, const u8 *filepath, const u8 *buf, const u64 size) {
  struct lexer lex = {
    .loc = {
      .at   = buf,
      .file = filepath,
      .line = 1,
      .len  = 1,
    }
  };

  struct token tok;
  do {
    next_token(&lex, &tok);
    xassert(tok_buf->num_tokens < MAX_NUM_TOKENS-1, "token_buffer out of space!");
    tok_buf->tokens[tok_buf->num_tokens++] = tok;
  } while (tok.type != END_OF_FILE);
}
