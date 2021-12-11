#define AST_NODE_MAX_CHILDREN 8

enum ast_node_type {
  AST_UNKNOWN,
  AST_CONSTANT,
  AST_TERM,
  AST_FACTOR,
  AST_UNARY_OP,
  AST_BINARY_OP,
  AST_VAR,
  AST_POSTFIX,
  AST_SUM,
  AST_FUN,
  AST_CREATE_OP,
  AST_ANNIHI_OP,
};

static const u8 * ast_node_names[] = {
  [AST_UNKNOWN] = "AST_UNKNOWN",
  [AST_CONSTANT] = "AST_CONSTANT",
  [AST_TERM] = "AST_TERM",
  [AST_FACTOR] = "AST_FACTOR",
  [AST_UNARY_OP] = "AST_UNARY_OP",
  [AST_BINARY_OP] = "AST_BINARY_OP",
  [AST_VAR] = "AST_VAR",
  [AST_POSTFIX] = "AST_POSTFIX",
  [AST_SUM] = "AST_SUM",
  [AST_FUN] = "AST_FUN",
  [AST_CREATE_OP] = "AST_CREATE_OP",
  [AST_ANNIHI_OP] = "AST_ANNIHI_OP",
};

struct ast_constant {
  int value;
};

struct ast_node {
  enum ast_node_type type;
  struct location loc;
  const u8 *name;
  union {
    struct ast_constant constant;
  };
  struct ast_node *children[AST_NODE_MAX_CHILDREN];
};

struct ast_node *ast_node_new() {
  void *p = malloc(sizeof(struct ast_node));
  if (!p) {
    die("Failed to malloc - %s\n", strerror(errno));
  }
  struct ast_node *node = p;
  memset(node, 0, sizeof(struct ast_node));
  return node;
}

static void dump_node_dot_recurse(struct ast_node *root, FILE *fd) {
  fprintf(fd, "NODE_%p [label=\"%s\\n%s\"];\n", root, ast_node_names[root->type], root->name);
  for (u8 i = 0; i < AST_NODE_MAX_CHILDREN; ++i) {
    if (root->children[i]) {
      dump_node_dot_recurse(root->children[i], fd);
    }
  }
}

static void dump_edge_dot_recurse(struct ast_node *root, FILE *fd) {
  for (u8 i = 0; i < AST_NODE_MAX_CHILDREN; ++i) {
    if (root->children[i]) {
      fprintf(fd, "NODE_%p -> NODE_%p\n", root, root->children[i]);
      dump_edge_dot_recurse(root->children[i], fd);
    }
  }
}

static void dump_ast_to_dot(struct ast_node *root, const u8 *filepath) {
  FILE *fd = fopen(filepath, "w");
  xassert(fd, "(fopen) %s\n", strerror(errno));

  fputs("digraph {\n", fd);
  dump_node_dot_recurse(root, fd);
  dump_edge_dot_recurse(root, fd);
  fputs("}\n", fd);

  fclose(fd);
}

static void dump_node_tex_recurse(struct ast_node *root, FILE *fd) {
  switch (root->type) {
  case AST_UNKNOWN:
    break;
  case AST_CONSTANT:
    fputs(root->name, fd);
    break;
  case AST_TERM:
  case AST_FACTOR:
  case AST_UNARY_OP:
    break;
  case AST_BINARY_OP: {
    dump_node_tex_recurse(root->children[0], fd);
    fputs(root->name, fd);
    dump_node_tex_recurse(root->children[1], fd);
  } break;
  case AST_VAR:
    fputs(root->name, fd);
    break;
  case AST_POSTFIX:
    break;
  case AST_SUM:
    fputs("\\sum_{", fd);
    dump_node_tex_recurse(root->children[0], fd);
    dump_node_tex_recurse(root->children[1], fd);
    dump_node_tex_recurse(root->children[2], fd);
    dump_node_tex_recurse(root->children[3], fd);
    fputs("}", fd);
    dump_node_tex_recurse(root->children[4], fd);
    break;
  case AST_FUN:
    break;
  case AST_CREATE_OP:
    fputs("\\hat{a}^\\dagger_{", fd);
    dump_node_tex_recurse(root->children[0], fd);
    fputs("}", fd);
    break;
  case AST_ANNIHI_OP:
    fputs("\\hat{a}_{", fd);
    dump_node_tex_recurse(root->children[0], fd);
    fputs("}", fd);
    break;
  };
}

static void dump_ast_to_tex(struct ast_node *root, const u8 *filepath) {
  FILE *fd = fopen(filepath, "w");
  xassert(fd, "(fopen) %s\n", strerror(errno));

  fputs("\\documentclass[varwidth,margin=2mm]{standalone}\n", fd);
  fputs("\\usepackage{amsmath}\n", fd);
  fputs("\\begin{document}\n", fd);
  fputs("\\begin{equation}\n", fd);

  dump_node_tex_recurse(root, fd);

  fputs("\\end{equation}\n", fd);
  fputs("\\end{document}\n", fd);

  fclose(fd);
}
