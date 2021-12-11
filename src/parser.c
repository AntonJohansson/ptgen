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

struct parser {
  struct token_buffer *tok_buf;
  u16 curr_tok;
};

static inline struct token *peek_token(struct parser *p) {
  return &p->tok_buf->tokens[p->curr_tok];
}

static inline struct token *pop_token(struct parser *p) {
  xassert(p->curr_tok < p->tok_buf->num_tokens, "pop_token, curr_tok out of bounds");
  return &p->tok_buf->tokens[p->curr_tok++];
}

static struct token *expect(struct parser *p, enum token_type tok_type) {
  struct token *t = pop_token(p);
  if (t->type != tok_type) {
    error("Token mismatch!\n");
    print_location(&t->loc, "Expected: %u\n", tok_type);
    die("Cannot recover!\n");
  }
  return t;
}

static bool match_either(struct parser *p, enum token_type *tok_types, u8 len) {
  for (u8 i = 0; i < len; ++i) {
    if (peek_token(p)->type == tok_types[i]) {
      return true;
    }
  }

  return false;
}

static bool match(struct parser *p, enum token_type tok) {
  if (peek_token(p)->type == tok) {
    return true;
  }

  return false;
}

static struct ast_node *parse_add(struct parser *p);

static struct ast_node *parse_reserved_function(struct parser *p) {
  struct token *tok_id = expect(p, IDENTIFIER);
  expect(p, LPAREN);
  struct ast_node *node_add = parse_add(p);
  expect(p, LPAREN);

  struct ast_node *node_fun = ast_node_new();
  node_fun->type = AST_FUN;
  node_fun->loc  = tok_id->loc;
  node_fun->name = xstrndup(tok_id->loc.at, tok_id->loc.len);
  node_fun->children[0] = node_add;

  return node_fun;
}

static struct ast_node *parse_iden(struct parser *p) {
  struct token *tok = expect(p, IDENTIFIER);

  struct ast_node *node_var = ast_node_new();
  node_var->type = AST_VAR;
  node_var->loc  = tok->loc;
  node_var->name = xstrndup(tok->loc.at, tok->loc.len);

  return node_var;
}

/* <primary-exp> ::= "(" <add-exp> ")" | <constant> | <id> | SUM(<id>,<id>,<id>,<id>){ <add-exp> } */
static struct ast_node *parse_primary(struct parser *p) {
  struct token *tok = peek_token(p);

  if (tok->type == LPAREN) {
    pop_token(p);
    struct ast_node *node_add = parse_add(p);
    expect(p, RPAREN);
    return node_add;
  } else if (tok->type == NUMBER) {
    pop_token(p);
    struct ast_node *node_constant = ast_node_new();
    node_constant->type = AST_CONSTANT;
    node_constant->loc  = tok->loc;
    node_constant->name = xstrndup(tok->loc.at, tok->loc.len);
    node_constant->constant.value = strtoull(tok->loc.at, NULL, 10);
    return node_constant;
  } else if (tok->type == IDENTIFIER) {
    struct ast_node *node_iden = parse_iden(p);
    return node_iden;
  } else if (tok->type == SUM) {
    pop_token(p);
    struct ast_node *node_sum = ast_node_new();
    node_sum->type = AST_SUM;
    node_sum->loc  = tok->loc;
    node_sum->name = xstrndup(tok->loc.at, tok->loc.len);

    expect(p, LPAREN);
    struct ast_node *node_id0 = parse_iden(p);
    expect(p, COMMA);
    struct ast_node *node_id1 = parse_iden(p);
    expect(p, COMMA);
    struct ast_node *node_id2 = parse_iden(p);
    expect(p, COMMA);
    struct ast_node *node_id3 = parse_iden(p);
    expect(p, RPAREN);

    expect(p, LBRACE);
    struct ast_node *node_add = parse_add(p);
    expect(p, RBRACE);

    node_sum->children[0] = node_id0;
    node_sum->children[1] = node_id1;
    node_sum->children[2] = node_id2;
    node_sum->children[3] = node_id3;
    node_sum->children[4] = node_add;

    return node_sum;
  } else if (tok->type == CREATE_OP) {
    pop_token(p);
    struct ast_node *node_create_op = ast_node_new();
    node_create_op->type = AST_CREATE_OP;
    node_create_op->loc  = tok->loc;
    node_create_op->name = xstrndup(tok->loc.at, tok->loc.len);

    expect(p, LPAREN);
    struct ast_node *node_id = parse_iden(p);
    expect(p, RPAREN);

    node_create_op->children[0] = node_id;

    return node_create_op;
  } else if (tok->type == ANNIHI_OP) {
    pop_token(p);
    struct ast_node *node_annihi_op = ast_node_new();
    node_annihi_op->type = AST_ANNIHI_OP;
    node_annihi_op->loc  = tok->loc;
    node_annihi_op->name = xstrndup(tok->loc.at, tok->loc.len);

    expect(p, LPAREN);
    struct ast_node *node_id = parse_iden(p);
    expect(p, RPAREN);

    node_annihi_op->children[0] = node_id;

    return node_annihi_op;
  } else {
    error("Unknown primary expression!\n");
    print_location(&tok->loc, "here\n");
    die("Cannot recover!\n");
  }
}

/* <postfix-exp> ::= <primary-exp> | <postfix-exp> "!" */
static struct ast_node *parse_postfix(struct parser *p) {
  struct ast_node *node_primary = parse_primary(p);

  /* TODO: we only support a single "!" */

  if (match(p, POW)) {
    struct token *tok_op = pop_token(p);
    struct ast_node *node_postfix = ast_node_new();
    node_postfix->type = AST_POSTFIX;
    node_postfix->loc = tok_op->loc;
    node_postfix->name = xstrndup(tok_op->loc.at, tok_op->loc.len);
    node_postfix->children[0] = node_primary;
    return node_postfix;
  } else {
    return node_primary;
  }
}

/* <unary-exp>  ::= <postfix-exp> | ("-" | "+") <unary-exp> */
static struct ast_node *parse_unary(struct parser *p) {
  if (match_either(p, (enum token_type[]){ADD,SUB}, 2)) {
    struct token *tok_op = pop_token(p);
    struct ast_node *node_unary = parse_unary(p);
    node_unary->type = AST_UNARY_OP;
    node_unary->loc = tok_op->loc;
    node_unary->name = xstrndup(tok_op->loc.at, tok_op->loc.len);
    node_unary->children[0] = node_unary;
    return node_unary;
  } else {
    return parse_postfix(p);
  }
}


typedef struct ast_node *parse_func(struct parser *);
static struct ast_node *parse_binary_op(struct parser *p, parse_func* pf, enum token_type *tok_ops, u8 len) {
  struct ast_node *e1 = pf(p);
  struct ast_node *e2 = NULL;
  struct ast_node *node_op = NULL;
  while (match_either(p, tok_ops, len)) {
    struct token *tok_op = pop_token(p);
    e2 = pf(p);

    node_op = ast_node_new();
    node_op->type = AST_BINARY_OP;
    node_op->name = xstrndup(tok_op->loc.at, tok_op->loc.len);
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
  struct token *tok_id  = expect(p, IDENTIFIER);
  struct token *tok_asn = expect(p, ASSIGN);
  struct ast_node *node_add = parse_add(p);
  struct ast_node *node_var = ast_node_new();
  node_var->type = AST_VAR;
  node_var->loc = tok_id->loc;
  node_var->name = xstrndup(tok_id->loc.at, tok_id->loc.len);

  struct ast_node *node_asn = ast_node_new();
  node_asn->type = AST_BINARY_OP;
  node_asn->loc = tok_asn->loc;
  node_asn->name = xstrndup(tok_asn->loc.at, tok_asn->loc.len);
  node_asn->children[0] = node_var;
  node_asn->children[1] = node_add;

  return node_asn;
}

static struct ast_node *parse(struct token_buffer *tok_buf) {
  struct parser p = {
    .tok_buf = tok_buf,
    .curr_tok = 0,
  };

  return parse_statement(&p);
}
