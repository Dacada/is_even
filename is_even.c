#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

struct symbol_inner {
    const char *str;
};

struct list_inner {
    struct thing *car;
    struct thing *cdr;
};

struct closure_inner {
    struct env *base_env;
    struct thing *arg_symbol;
    struct thing *body;
};

enum type {
    TYPE_SYMBOL,
    TYPE_LIST,
    TYPE_CLOSURE,
};

struct thing {
    enum type type;
};

struct closure {
    struct thing base;
    struct closure_inner inner;
};

struct closure_inner *closure_extract(struct thing *closure) {
    assert(closure != NULL);
    assert(closure->type == TYPE_CLOSURE);
    return &((struct closure *)closure)->inner;
}

struct thing *closure_get_body(struct thing *closure) {
    struct closure_inner *inner = closure_extract(closure);
    return inner->body;
}

struct env *closure_get_base_env(struct thing *closure) {
    struct closure_inner *inner = closure_extract(closure);
    return inner->base_env;
}

struct thing *closure_get_arg_symbol(struct thing *closure) {
    struct closure_inner *inner = closure_extract(closure);
    return inner->arg_symbol;
}

struct thing *eval_inner(struct thing *expr, struct env *env);
struct env *env_new(struct thing *closure, struct thing *arg);
struct thing *closure_apply(struct thing *closure, struct thing *arg) {
    struct env *env = env_new(closure, arg);
    return eval_inner(closure_get_body(closure), env);
}

struct thing *closure_new(struct env *env, struct thing *symbol, struct thing *body) {
    struct closure *thing = malloc(sizeof(*thing));
    thing->base.type = TYPE_CLOSURE;
    thing->inner.base_env = env;
    thing->inner.arg_symbol = symbol;
    thing->inner.body = body;
    return &thing->base;
}

struct symbol {
    struct thing base;
    struct symbol_inner inner;
};

struct thing *symbol_new(const char *str) {
    struct symbol *thing = malloc(sizeof(*thing));
    thing->base.type = TYPE_SYMBOL;
    thing->inner.str = str;
    return &thing->base;
}

const char *symbol_extract(struct thing *symbol) {
    assert(symbol != NULL);
    assert(symbol->type == TYPE_SYMBOL);
    return ((struct symbol *)symbol)->inner.str;
}

bool symbol_is(struct thing *thing) {
    return thing != NULL && thing->type == TYPE_SYMBOL;
}

bool symbol_eq(struct thing *s1, struct thing *s2) {
    return strcmp(symbol_extract(s1), symbol_extract(s2)) == 0;
}

bool symbol_eq_static(struct thing *s1, const char *s2) {
    return strcmp(symbol_extract(s1), s2) == 0;
} 

struct list {
    struct thing base;
    struct list_inner inner;
};

// input must be reversed
struct thing *list_new(int count, ...) {
    va_list args;
    va_start(args, count);

    struct thing *curr = NULL;
    for (int i = 0; i < count; i++) {
        struct list *next = malloc(sizeof(*next));
        next->base.type = TYPE_LIST;
        next->inner.car = va_arg(args, struct thing *);
        next->inner.cdr = curr;
        curr = &next->base;
    }

    va_end(args);

    return curr;
}

bool list_empty(struct thing *list) {
    return list == NULL;
}

struct list_inner *list_extract(struct thing *list) {
    assert(list != NULL);
    assert(list->type == TYPE_LIST);
    return &((struct list *)list)->inner;
}

struct thing *list_first(struct thing *list) {
    struct list_inner *inner = list_extract(list);
    return inner->car;
}

struct thing *list_rest(struct thing *list) {
    struct list_inner *inner = list_extract(list);
    return inner->cdr;
}

struct env {
    struct env *base;
    struct thing *symbol;
    struct thing *value;
};

struct env *env_new(struct thing *closure, struct thing *arg) {
    struct env *env = malloc(sizeof(*env));
    env->base = closure_get_base_env(closure);
    env->symbol = closure_get_arg_symbol(closure);
    env->value = arg;
    return env;
}

struct thing *env_lookup(struct env *env, struct thing *symbol) {
    assert(env != NULL);

    if (symbol_eq(env->symbol, symbol)) {
        return env->value;
    }
    return env_lookup(env->base, symbol);
}

struct thing *eval_inner(struct thing *expr, struct env *env) {
    if (list_empty(expr)) {
        return expr;
    }

    if (expr->type == TYPE_CLOSURE) {
        return expr;
    }

    if (expr->type == TYPE_SYMBOL) {
        return env_lookup(env, expr);
    }

    struct thing *first = list_first(expr);
    struct thing *rest = list_rest(expr);
    if (symbol_is(first) && symbol_eq_static(first, "lambda")) {
        struct thing *arg = list_first(rest);
        struct thing *body = list_first(list_rest(rest));
        return closure_new(env, arg, body);
    }

    struct thing *second = list_first(rest);
    struct thing *arg = eval_inner(second, env);

    struct thing *closure = eval_inner(first, env);
    return closure_apply(closure, arg);
}

struct thing *eval(struct thing *expr) {
    return eval_inner(expr, NULL);
}

void test_expression(struct thing *TRUE, struct thing *FALSE, struct thing *expr, bool to_be) {
    struct thing *a = eval(TRUE);
    struct thing *b = eval(FALSE);

    struct thing *res = to_be ? a : b;

    struct thing *result = eval(expr);
    assert(closure_apply(closure_apply(result, a), b) == res);

}

int main() {
    struct thing *TRUE = list_new(
        3,
        list_new(
            3,
            symbol_new("a"),
            symbol_new("b"),
            symbol_new("lambda")
        ),
        symbol_new("a"),
        symbol_new("lambda")
    );

    struct thing *FALSE = list_new(
        3,
        list_new(
            3,
            symbol_new("b"),
            symbol_new("b"),
            symbol_new("lambda")
        ),
        symbol_new("a"),
        symbol_new("lambda")
    );

    struct thing *NOT = list_new(
        3,
        list_new(
            2,
            TRUE,
            list_new(
                2,
                FALSE,
                symbol_new("p")
            )
        ),
        symbol_new("p"),
        symbol_new("lambda")
    );

    struct thing *IS_EVEN = list_new(
        3,
        list_new(
            2,
            TRUE,
            list_new(
                2,
                NOT,
                symbol_new("n")
            )
        ),
        symbol_new("n"),
        symbol_new("lambda")
    );

    struct thing *TWO = list_new(
        3,
        list_new(
            3,
            list_new(
                2,
                list_new(
                    2,
                    symbol_new("x"),
                    symbol_new("f")
                ),
                symbol_new("f")
            ),
            symbol_new("x"),
            symbol_new("lambda")
        ),
        symbol_new("f"),
        symbol_new("lambda")
    );

    struct thing *THREE = list_new(
        3,
        list_new(
            3,
            list_new(
                2,
                list_new(
                    2,
                    list_new(
                        2,
                        symbol_new("x"),
                        symbol_new("f")
                    ),
                    symbol_new("f")
                ),
                symbol_new("f")
            ),
            symbol_new("x"),
            symbol_new("lambda")
        ),
        symbol_new("f"),
        symbol_new("lambda")
    );

    test_expression(TRUE, FALSE, list_new(2, THREE, IS_EVEN), false);
    test_expression(TRUE, FALSE, list_new(2, TWO, IS_EVEN), true);

    return 0;
}
