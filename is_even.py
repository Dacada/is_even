TRUE = ["fun", "a", ["fun", "b", "a"]]
FALSE = ["fun", "a", ["fun", "b", "b"]]
NOT = ["fun", "p", [["p", FALSE], TRUE]]
IS_EVEN = ["fun", "n", [["n", NOT], TRUE]]

ZERO = ["fun", "f", ["fun", "x", "x"]]
ONE = ["fun", "f", ["fun", "x", ["f", "x"]]]
TWO = ["fun", "f", ["fun", "x", ["f",  ["f", "x"]]]]
THREE = ["fun", "f", ["fun", "x", ["f",  ["f", ["f", "x"]]]]]


def f(expr, env):
    # variable lookup in environment
    if isinstance(expr, str):
        return env(expr)

    # evaluation
    if isinstance(expr, list):
        # empty list evaluates to empty list
        if len(expr) == 0:
            return expr

        first = expr[0]
        rest = expr[1:]

        # function definition
        if first == "fun":
            fun_arg = rest[0]
            fun_body = rest[1]

            def newfun(arg):
                def newenv(y):
                    if y == fun_arg:
                        return arg
                    else:
                        return env(y)
                return f(fun_body, newenv)
            return newfun

        # function application
        return f(first, env)(f(rest[0], env))


def empty_env(x):
    raise Exception(f"{x} unbound")


def ff(expr):
    return f(expr, empty_env)


assert ff([IS_EVEN, THREE])(True)(False) is (3 % 2 == 0)
assert ff([IS_EVEN, TWO])(True)(False) is (2 % 2 == 0)
