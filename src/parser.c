/*
 * complete parser syntax:
 *   <program> ::= <statement>
 *
 *   <statement> ::= <id> <assignment-op> <exp> | <add-exp>
 *
 *   <add-exp> ::= <mul-exp> { ("+" | "-") <mul-exp> }
 *
 *   <mul-exp>  ::= <pow-exp> {("*" | "/") <pow-exp>}
 *
 *   <pow-exp> ::= <unary-exp> { "^" <unary-exp> }
 *
 *   <unary-exp>  ::= <postfix-exp> | ("-" | "+") <unary-exp>
 *
 *   <postfix-exp> ::= <primary-exp> | <postfix-exp> "!"
 *
 *   <primary-exp> ::= "(" <comma-exp> ")" | <id>
 */

#define TOKEN_BUF_LEN 16

struct token_buffer {
    struct token toks[TOKEN_BUF_LEN];
    u8 begin;
    u8 end;
    u8 len;
};

void push_token(struct token_buffer *buf, struct token *tok) {
    if (buf->len >= TOKEN_BUF_LEN) {
        die("Ran out of token buffer space!\n");
    }

    buf->toks[buf->end] = *tok;
    buf->end = (buf->end + 1) % TOKEN_BUF_LEN;
    buf->len++;
}

void pop_token(struct token_buffer *buf, struct token *tok) {
    if (buf->len == 0) {
        die("Cannot pop empty buffer!\n");
    }

    *tok = buf->toks[buf->begin];
    buf->begin = (buf->begin + 1) % TOKEN_BUF_LEN;
    buf->len--;
}

static void expect(struct token_buffer *buf, enum token_type tok_type) {
    if (buf->toks[buf->begin].type != tok_type) {
        error("Token mismatch!\n");
        print_location(&buf->toks[buf->begin].loc, "Expected: %u\n", tok_type);
        die("Cannot recover!\n");
    }
}

static bool match_seq(struct token_buffer *buf, enum token_type *tok_types, u8 len) {
    if (len > buf->len) {
        return false;
    }

    for (u8 i = 0; i < len; ++i) {
        if (buf->toks[(buf->begin + i) % TOKEN_BUF_LEN].type != tok_types[i]) {
            return false;
        }
    }

    return true;
}

/* <statement> ::= <id> <assignment-op> <exp> | <add-exp> */
static void parse_statement() {
}
/* <add-exp> ::= <mul-exp> { ("+" | "-") <mul-exp> } */
static void parse_add() {
}
/* <mul-exp>  ::= <pow-exp> {("*" | "/") <pow-exp>} */
static void parse_mul() {
}
/* <pow-exp> ::= <unary-exp> { "^" <unary-exp> } */
void parse_pow() {
}
/* <unary-exp>  ::= <postfix-exp> | ("-" | "+") <unary-exp> */
void parse_unary() {
}
/* <postfix-exp> ::= <primary-exp> | <postfix-exp> "!" */
void parse_postfix() {
}
/* <primary-exp> ::= "(" <add-exp> ")" | <id> */
void parse_primary() {
}
