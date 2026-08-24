#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

struct symbol_inner {
    const char *str;
};

struct cons_inner {
    struct object *car;
    struct object *cdr;
};

struct closure_inner {
    struct object *base_env;
    struct object *arg_symbol;
    struct object *body;
};

enum type {
    TYPE_SYMBOL,
    TYPE_CONS,
    TYPE_CLOSURE,
};

struct object {
    enum type type;
};

struct closure {
    struct object base;
    struct closure_inner inner;
};

struct closure_inner *closure_extract(struct object *closure) {
    assert(closure != NULL);
    assert(closure->type == TYPE_CLOSURE);
    return &((struct closure *)closure)->inner;
}

struct object *closure_get_body(struct object *closure) {
    struct closure_inner *inner = closure_extract(closure);
    return inner->body;
}

struct object *closure_get_base_env(struct object *closure) {
    struct closure_inner *inner = closure_extract(closure);
    return inner->base_env;
}

struct object *closure_get_arg_symbol(struct object *closure) {
    struct closure_inner *inner = closure_extract(closure);
    return inner->arg_symbol;
}

struct object *eval_inner(struct object *expr, struct object *env);
struct object *env_new(struct object *closure, struct object *arg);
struct object *closure_apply(struct object *closure, struct object *arg) {
    struct object *env = env_new(closure, arg);
    return eval_inner(closure_get_body(closure), env);
}

struct object *closure_new(struct object *env, struct object *symbol, struct object *body) {
    struct closure *object = malloc(sizeof(*object));
    object->base.type = TYPE_CLOSURE;
    object->inner.base_env = env;
    object->inner.arg_symbol = symbol;
    object->inner.body = body;
    return &object->base;
}

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

// input must be reversed
struct object *list_new(int count, ...) {
    va_list args;
    va_start(args, count);

    struct object *curr = NULL;
    for (int i = 0; i < count; i++) {
        curr = cons_new(
            va_arg(args, struct object *),
            curr
        );
    }

    va_end(args);

    return curr;
}

struct object *env_new(struct object *closure, struct object *arg) {
    struct object *binding = cons_new(
        closure_get_arg_symbol(closure),
        arg
    );
    
    struct object *env = cons_new(
        binding,
        closure_get_base_env(closure)
    );
    
    return env;
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

struct object *eval_inner(struct object *expr, struct object *env) {
    if (expr == NULL) {
        return expr;
    }

    if (expr->type == TYPE_CLOSURE) {
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

    struct object *second = cons_car(rest);
    struct object *arg = eval_inner(second, env);

    struct object *closure = eval_inner(first, env);
    return closure_apply(closure, arg);
}

struct object *eval(struct object *expr) {
    return eval_inner(expr, NULL);
}

void test_expression(struct object *TRUE, struct object *FALSE, struct object *expr, bool to_be) {
    struct object *a = eval(TRUE);
    struct object *b = eval(FALSE);

    struct object *res = to_be ? a : b;

    struct object *result = eval(expr);
    assert(closure_apply(closure_apply(result, a), b) == res);

}

int main() {
    struct object *TRUE = list_new(
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

    struct object *FALSE = list_new(
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

    struct object *NOT = list_new(
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

    struct object *IS_EVEN = list_new(
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

    struct object *TWO = list_new(
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

    struct object *THREE = list_new(
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
