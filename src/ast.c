#define AST_NODE_MAX_CHILDREN 8

enum ast_node_type {
    AST_UNKNOWN = 0,
    AST_CONSTANT,
    AST_TERM,
    AST_FACTOR,
    AST_UNARY_OP,
    AST_BINARY_OP,
    AST_VAR,
    AST_POSTFIX,
    AST_SUM,
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
