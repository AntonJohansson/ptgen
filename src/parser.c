/*
 * complete parser syntax:
 *   <program> ::= <statement>
 *
 *   <statement> ::= <id> <assignment-op> <add-exp>
 *
 *   <add-exp> ::= <mul-exp> { ("+" | "-") <mul-exp> }
 *
 *   <mul-exp>  ::= <pow-exp> {("*" | "/") <pow-exp>}
 *
 *   <pow-exp> ::= <unary-exp> { "^" <unary-exp> }
 *
 *   <unary-exp>  ::= <postfix-exp> | ("-" | "+") <unary-exp>
 *
 *   <postfix-exp> ::= <primary-exp> | <primary-exp> "!"
 *
 *   <primary-exp> ::= "(" <add-exp> ")" | <constant> | <call-exp> | <sum-exp> | <id>
 *
 *   <sum-exp> ::= "SUM" "(" <id-list-exp> ")" "{" <add-exp> "}"
 *
 *   <id-list-exp> ::= <id> { "," <id> }
 *
 *   <call-exp> ::= <id> "(" <param-exp> ")"
 *
 *   <param-exp> ::= <add-exp> { "," <add-exp> }
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

    if (tok) {
        *tok = buf->toks[buf->begin];
    }

    buf->begin = (buf->begin + 1) % TOKEN_BUF_LEN;
    buf->len--;
}

void peep_token(struct token_buffer *buf, struct token *tok) {
    if (buf->len == 0) {
        die("Cannot peek empty buffer!\n");
    }

    if (!tok) {
        die("Need to supply tok!\n");
    }

    *tok = buf->toks[buf->begin];
}

struct parser {
    struct lexer lex;
    struct token_buffer buf;
};

static void fetch_tokens(struct parser *p, u8 amount) {
    if (amount + p->buf.len > TOKEN_BUF_LEN) {
        die("Requested more tokens than available space in buffer\n");
    }

    struct token tok;
    for (u8 i = 0; i < amount; ++i) {
        next_token(&p->lex, &tok);
        if (tok.type == END_OF_FILE) {
            break;
        }
        push_token(&p->buf, &tok);
    }
}

static void fetch_tokens_if_needed(struct parser *p, u8 amount) {
    if (p->buf.len < amount) {
        fetch_tokens(p, amount - p->buf.len);
    }
}

static void assert_space_in_buffer(struct token_buffer *buf, u8 len) {
    if (len > buf->len) {
        die("Not enough space in buffer, requested %u, have %u!\n", len, buf->len);
    }
}

static void expect(struct token_buffer *buf, enum token_type tok_type, struct token *tok) {
    assert_space_in_buffer(buf, 1);
    struct token t;
    pop_token(buf, &t);
    if (t.type != tok_type) {
        error("Token mismatch!\n");
        print_location(&buf->toks[buf->begin].loc, "Expected: %u\n", tok_type);
        die("Cannot recover!\n");
    }
    if (tok) {
        *tok = t;
    }
}

static bool match_seq(struct token_buffer *buf, enum token_type *tok_types, u8 len) {
    assert_space_in_buffer(buf, len);

    for (u8 i = 0; i < len; ++i) {
        if (buf->toks[(buf->begin + i) % TOKEN_BUF_LEN].type != tok_types[i]) {
            return false;
        }
    }

    return true;
}

static bool match_either(struct token_buffer *buf, enum token_type *tok_types, u8 len) {
    assert_space_in_buffer(buf, 1);

    for (u8 i = 0; i < len; ++i) {
        if (buf->toks[(buf->begin + i) % TOKEN_BUF_LEN].type == tok_types[i]) {
            return true;
        }
    }

    return false;
}

static bool match(struct token_buffer *buf, enum token_type tok) {
    assert_space_in_buffer(buf, 1);

    if (buf->toks[buf->begin].type == tok) {
        return true;
    }

    return false;
}

static struct ast_node *parse_add(struct parser *p);

static struct ast_node *parse_reserved_function(struct parser *p) {
    fetch_tokens_if_needed(p, 2);
    struct token tok_id;
    expect(&p->buf, IDENTIFIER, &tok_id);
    expect(&p->buf, LPAREN, NULL);
    struct ast_node *node_add = parse_add(p);
    fetch_tokens_if_needed(p, 1);
    expect(&p->buf, LPAREN, NULL);

    struct ast_node *node_fun = ast_node_new();
    node_fun->type = AST_FUN;
    node_fun->loc  = tok.loc;
    node_fun->name = xstrndup(tok.loc.at, tok.loc.len);
    node_fun->children[0] = node_add;

    return node_fun;
}

static struct ast_node *parse_iden(struct parser *p) {
    fetch_tokens_if_needed(p, 1);
    struct token tok;
    expect(&p->buf, IDENTIFIER, &tok);

    struct ast_node *node_var = ast_node_new();
    node_var->type = AST_VAR;
    node_var->loc  = tok.loc;
    node_var->name = xstrndup(tok.loc.at, tok.loc.len);

    return node_var;
}

/* <primary-exp> ::= "(" <add-exp> ")" | <constant> | <id> | SUM(<id>,<id>,<id>,<id>){ <add-exp> } */
static struct ast_node *parse_primary(struct parser *p) {
    struct token tok;
    fetch_tokens_if_needed(p, 1);
    peep_token(&p->buf, &tok);

    if (tok.type == LPAREN) {
        pop_token(&p->buf, NULL);
        struct ast_node *node_add = parse_add(p);
        fetch_tokens_if_needed(p, 1);
        expect(&p->buf, RPAREN, NULL);
        return node_add;
    } else if (tok.type == NUMBER) {
        pop_token(&p->buf, NULL);
        struct ast_node *node_constant = ast_node_new();
        node_constant->type = AST_CONSTANT;
        node_constant->loc  = tok.loc;
        node_constant->name = "integer";
        node_constant->constant.value = strtoull(tok.loc.at, NULL, 10);
        return node_constant;
    } else if (tok.type == IDENTIFIER) {
        struct ast_node *node_iden = parse_iden(p);
        return node_iden;
    } else if (tok.type == SUM) {
        pop_token(&p->buf, NULL);
        struct ast_node *node_sum = ast_node_new();
        node_sum->type = AST_SUM;
        node_sum->loc  = tok.loc;
        node_sum->name = xstrndup(tok.loc.at, tok.loc.len);

        fetch_tokens_if_needed(p, 10);
        expect(&p->buf, LPAREN, NULL);
        struct ast_node *node_id0 = parse_iden(p);
        expect(&p->buf, COMMA, NULL);
        struct ast_node *node_id1 = parse_iden(p);
        expect(&p->buf, COMMA, NULL);
        struct ast_node *node_id2 = parse_iden(p);
        expect(&p->buf, COMMA, NULL);
        struct ast_node *node_id3 = parse_iden(p);
        expect(&p->buf, RPAREN, NULL);

        fetch_tokens_if_needed(p, 1);
        expect(&p->buf, LBRACE, NULL);
        struct ast_node *node_add = parse_add(p);
        fetch_tokens_if_needed(p, 1);
        expect(&p->buf, RBRACE, NULL);

        node_sum->children[0] = node_id0;
        node_sum->children[1] = node_id1;
        node_sum->children[2] = node_id2;
        node_sum->children[3] = node_id3;
        node_sum->children[4] = node_add;

        return node_sum;
    } else {
        error("Unknown primary expression!\n");
        print_location(&tok.loc, "here\n");
        die("Cannot recover!\n");
    }
}

/* <postfix-exp> ::= <primary-exp> | <postfix-exp> "!" */
static struct ast_node *parse_postfix(struct parser *p) {
    struct ast_node *node_primary = parse_primary(p);

    /* TODO: we only support a single "!" */

    fetch_tokens_if_needed(p, 1);
    if (match(&p->buf, POW)) {
        struct token tok_op;
        pop_token(&p->buf, &tok_op);
        struct ast_node *node_postfix = ast_node_new();
        node_postfix->type = AST_POSTFIX;
        node_postfix->loc = tok_op.loc;
        node_postfix->name = xstrndup(tok_op.loc.at, tok_op.loc.len);
        node_postfix->children[0] = node_primary;
        return node_postfix;
    } else {
        return node_primary;
    }
}

/* <unary-exp>  ::= <postfix-exp> | ("-" | "+") <unary-exp> */
static struct ast_node *parse_unary(struct parser *p) {
    fetch_tokens_if_needed(p, 1);
    if (match_either(&p->buf, (enum token_type[]){ADD,SUB}, 2)) {
        struct token tok_op;
        pop_token(&p->buf, &tok_op);
        struct ast_node *node_unary = parse_unary(p);
        node_unary->type = AST_UNARY_OP;
        node_unary->loc = tok_op.loc;
        node_unary->name = xstrndup(tok_op.loc.at, tok_op.loc.len);
        node_unary->children[0] = node_unary;
        return node_unary;
    } else {
        return parse_postfix(p);
    }
}


typedef struct ast_node *parse_func(struct parser *);
static struct ast_node *parse_binary_op(struct parser *p, parse_func* pf, enum token_type *tok_ops, u8 len) {
    struct token tok_op;
    struct ast_node *e1 = pf(p);
    struct ast_node *e2 = NULL;
    struct ast_node *node_op = NULL;
    fetch_tokens_if_needed(p, 1);
    while (match_either(&p->buf, tok_ops, len)) {
        pop_token(&p->buf, &tok_op);
        e2 = pf(p);
        fetch_tokens_if_needed(p, 1);

        node_op = ast_node_new();
        node_op->type = AST_BINARY_OP;
        node_op->name = xstrndup(tok_op.loc.at, tok_op.loc.len);
        node_op->children[0] = e1;
        node_op->children[1] = e2;
        e1 = node_op;
    }

    return e1;
}

/* <pow-exp> ::= <unary-exp> { "^" <unary-exp> } */
static struct ast_node *parse_pow(struct parser *p) {
    return parse_binary_op(p, parse_unary, (enum token_type[]){POW}, 2);
}

/* <mul-exp>  ::= <pow-exp> {("*" | "/") <pow-exp>} */
static struct ast_node *parse_mul(struct parser *p) {
    return parse_binary_op(p, parse_pow, (enum token_type[]){MUL, DIV}, 2);
}

/* <add-exp> ::= <mul-exp> { ("+" | "-") <mul-exp> } */
static struct ast_node *parse_add(struct parser *p) {
    return parse_binary_op(p, parse_mul, (enum token_type[]){ADD, SUB}, 2);
}

/* <statement> ::= <id> <assignment-op> <add-exp> */
static struct ast_node *parse_statement(struct parser *p) {
    struct token tok_id, tok_asn;
    fetch_tokens_if_needed(p, 2);
    expect(&p->buf, IDENTIFIER, &tok_id);
    expect(&p->buf, ASSIGN, &tok_asn);
    struct ast_node *node_add = parse_add(p);
    struct ast_node *node_var = ast_node_new();
    node_var->type = AST_VAR;
    node_var->loc = tok_id.loc;
    node_var->name = xstrndup(tok_id.loc.at, tok_id.loc.len);

    struct ast_node *node_asn = ast_node_new();
    node_asn->type = AST_BINARY_OP;
    node_asn->loc = tok_asn.loc;
    node_asn->name = xstrndup(tok_id.loc.at, tok_id.loc.len);
    node_asn->children[0] = node_var;
    node_asn->children[1] = node_add;

    return node_asn;
}

static void parse(const u8 *filepath, const u8 *buf, const u64 size) {
    struct parser p = {
        .lex = {
            .loc = {
                .at   = buf,
                .file = filepath,
                .line = 1,
                .len  = 1,
            },
        },
        .buf = {0},
    };

    parse_statement(&p);
}
