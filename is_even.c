#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

struct symbol_inner {
    const char *str;
};

struct cons_inner {
    struct object *car;
    struct object *cdr;
};

enum type {
    TYPE_SYMBOL,
    TYPE_CONS,
};

struct object {
    enum type type;
};

struct symbol {
    struct object base;
    struct symbol_inner inner;
};

struct object *symbol_new(const char *str) {
    struct symbol *object = malloc(sizeof(*object));
    object->base.type = TYPE_SYMBOL;
    object->inner.str = str;
    return &object->base;
}

const char *symbol_extract(struct object *symbol) {
    assert(symbol != NULL);
    assert(symbol->type == TYPE_SYMBOL);
    return ((struct symbol *)symbol)->inner.str;
}

bool symbol_is(struct object *object) {
    return object != NULL && object->type == TYPE_SYMBOL;
}

bool symbol_eq(struct object *s1, struct object *s2) {
    return strcmp(symbol_extract(s1), symbol_extract(s2)) == 0;
}

bool symbol_eq_static(struct object *s1, const char *s2) {
    return strcmp(symbol_extract(s1), s2) == 0;
} 

struct cons {
    struct object base;
    struct cons_inner inner;
};

struct cons_inner *cons_extract(struct object *cons) {
    assert(cons != NULL);
    assert(cons->type == TYPE_CONS);
    return &((struct cons *)cons)->inner;
}

struct object *cons_new(struct object *car, struct object *cdr) {
    struct cons *object = malloc(sizeof(*object));
    object->base.type = TYPE_CONS;
    object->inner.car = car;
    object->inner.cdr = cdr;
    return &object->base;
}

struct object *cons_car(struct object *cons) {
    struct cons_inner *inner = cons_extract(cons);
    return inner->car;
}

struct object *cons_cdr(struct object *cons) {
    struct cons_inner *inner = cons_extract(cons);
    return inner->cdr;
}

struct object *closure_get_body(struct object *closure) {
    struct object *function = cons_cdr(closure);
    return cons_cdr(function);
}

struct object *closure_get_base_env(struct object *closure) {
    return cons_car(closure);
}

struct object *closure_get_arg_symbol(struct object *closure) {
    struct object *function = cons_cdr(closure);
    return cons_car(function);
}

struct object *closure_new(struct object *env, struct object *symbol, struct object *body) {
    struct object *function = cons_new(symbol, body);
    return cons_new(env, function);
}

struct object *eval(struct object *expr, struct object *env);
struct object *env_from_closure(struct object *closure, struct object *arg);
struct object *closure_apply(struct object *closure, struct object *arg) {
    struct object *env = env_from_closure(closure, arg);
    return eval(closure_get_body(closure), env);
}

struct object *env_bind(struct object *env, struct object *symbol, struct object *value) {
    struct object *binding = cons_new(symbol, value);
    return cons_new(binding, env);
}

struct object *env_from_closure(struct object *closure, struct object *arg) {
    return env_bind(
        closure_get_base_env(closure),
        closure_get_arg_symbol(closure),
        arg
    );
}

struct object *env_lookup(struct object *env, struct object *symbol) {
    assert(env != NULL);
    
    struct object *binding = cons_car(env);
    struct object *env_symbol = cons_car(binding);

    if (symbol_eq(env_symbol, symbol)) {
        return cons_cdr(binding);
    }
    
    return env_lookup(cons_cdr(env), symbol);
}

struct object *eval(struct object *expr, struct object *env) {
    if (expr == NULL) {
        return expr;
    }

    if (expr->type == TYPE_SYMBOL) {
        return env_lookup(env, expr);
    }

    struct object *first = cons_car(expr);
    struct object *rest = cons_cdr(expr);
    if (symbol_is(first) && symbol_eq_static(first, "lambda")) {
        struct object *arg = cons_car(rest);
        struct object *body = cons_car(cons_cdr(rest));
        return closure_new(env, arg, body);
    }
    
    struct object *closure = eval(first, env);

    struct object *second = cons_car(rest);
    struct object *arg = eval(second, env);

    return closure_apply(closure, arg);
}

struct object *parse_inner(const unsigned char **src);

void parse_skip_whitespace(const unsigned char **src) {
    while (isblank(**src)) {
        *src += 1;
        if (**src == '\0') {
            return;
        }
    }
}

struct object *parse_list_rest(const unsigned char **src) {
    struct object *list = NULL;
    struct object **next = &list;
    
    while (**src != ')') {
        struct object *obj = parse_inner(src);
        parse_skip_whitespace(src);
        
        struct object *new = cons_new(obj, NULL);
        *next = new;
        next = &cons_extract(new)->cdr;
        
    }
    
    *src += 1;
    return list;
}

struct object *parse_list_or_cons(const unsigned char **src) {
    // known open parenthesis
    *src += 1;
    
    parse_skip_whitespace(src);
    
    if (**src == ')') {
        // empty list
        *src += 1;
        return NULL;
    }
    
    struct object *first = parse_inner(src);
    
    parse_skip_whitespace(src);
    
    if (**src != '.') {
        struct object *rest = parse_list_rest(src);
        return cons_new(first, rest);
    }
    
    *src += 1;
    
    parse_skip_whitespace(src);
    
    struct object *second = parse_inner(src);
    struct object *cons = cons_new(first, second);
    
    parse_skip_whitespace(src);
    
    if (**src == ')') {
        *src += 1;
        return cons;
    }
    
    assert(false);
}

struct object *parse_atom(const unsigned char **src) {
    const unsigned char *orig = *src;
    while (isalpha(**src)) {
        *src += 1;
    }
    size_t size = sizeof(char) * (*src - orig);
    char *name = malloc(size + 1);
    memcpy(name, orig, size);
    name[size] = '\0';
    return symbol_new(name);
}

struct object *parse_inner(const unsigned char **src) {
    parse_skip_whitespace(src);
    assert(**src != '\0');
    
    if (**src == '(') {
        return parse_list_or_cons(src);
    }
    
    if (isalpha(**src)) {
        return parse_atom(src);
    }
    
    assert(false);
}

struct object *parse(const char *src) {
    const unsigned char *ptr = ((const unsigned char*)src);
    return parse_inner(&ptr);
}

void test_expression(struct object *env, struct object *expr, bool to_be) {
    struct object *t = symbol_new("TRUE");
    struct object *f = symbol_new("FALSE");

    struct object *tt = env_lookup(env, t);
    struct object *ff = env_lookup(env, f);

    struct object *res = to_be ? tt : ff;

    struct object *result = eval(expr, env);
    assert(closure_apply(closure_apply(result, tt), ff) == res);
}

struct object *env_bind_char(struct object *env, const char *str, struct object *val) {
    struct object *symbol = symbol_new(str);
    return env_bind(env, symbol, val);
}

int main() {
    struct object *TRUE = parse("(lambda a (lambda b a))");
    struct object *FALSE = parse("(lambda a (lambda b b))");
    struct object *NOT = parse("(lambda p ((p FALSE) TRUE))");
    struct object *ISEVEN = parse("(lambda n ((n NOT) TRUE))");
    struct object *TWO = parse("(lambda f (lambda x (f (f x))))");
    struct object *THREE = parse("(lambda f (lambda x (f (f (f x)))))");

    struct object *IS_THREE_EVEN = parse("(ISEVEN THREE)");
    struct object *IS_TWO_EVEN = parse("(ISEVEN TWO)");

    struct object *env = NULL;

    env = env_bind_char(env, "TRUE", eval(TRUE, env));
    env = env_bind_char(env, "FALSE", eval(FALSE, env));
    env = env_bind_char(env, "NOT", eval(NOT, env));
    env = env_bind_char(env, "ISEVEN", eval(ISEVEN, env));
    env = env_bind_char(env, "TWO", eval(TWO, env));
    env = env_bind_char(env, "THREE", eval(THREE, env));

    test_expression(env, IS_THREE_EVEN, false);
    test_expression(env, IS_TWO_EVEN, true);

    return 0;
}
