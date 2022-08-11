#ifndef ZEN_MACROS_H
#define ZEN_MACROS_H

#include "zen_config.h"

// Ye' Olde stringify
#define ZEN_STRINGIFY(x)    _stringify_impl(x)
#define _stringify_impl(x)  #x

// Basic identity macro
#ifndef IDENTITY
#define IDENTITY(x) x
#endif

// Preprocessor pack value extraction
//      PACK_GET(2, (a, b, c, d))   ->      c
#define PACK_GET(N, v) _pack_get ## N v
#define _pack_get0(x0, ...) x0
#define _pack_get1(x0, x1, ...) x1
#define _pack_get2(x0, x1, x2, ...) x2
#define _pack_get3(x0, x1, x2, x3, ...) x3
#define _pack_get4(x0, x1, x2, x3, x4, ...) x4
#define _pack_get5(x0, x1, x2, x3, x4, x5, ...) x5
#define _pack_get6(x0, x1, x2, x3, x4, x5, x6, ...) x6
#define _pack_get7(x0, x1, x2, x3, x4, x5, x6, x7, ...) x7


// For each macro (4 variants)
//    FOR_EACH(F, ...)              ->      F(index, value)...
//    FOR_EACH_ARG(F, A, ...)       ->      F(A, index, value)...
//    FOR_EACH_FOLD(F, S, ...)      ->      F(index, value) S ...
//    FOR_EACH_COMMA(F, ...)        ->      F(index, value) , ...
#ifdef ZEN_CPP20

#define FOR_EACH(macro, ...)            __VA_OPT__(_expand(_for_each_helper(macro, 0, __VA_ARGS__)))
#define FOR_EACH_ARG(macro, arg, ...)   __VA_OPT__(_expand(_for_each_arg_helper(macro, arg, 0, __VA_ARGS__)))
#define FOR_EACH_FOLD(macro, sep, ...)  __VA_OPT__(_expand(_for_each_fold_helper(macro, sep, 0, __VA_ARGS__)))
#define FOR_EACH_COMMA(macro, ...)      __VA_OPT__(_expand(_for_each_cma_helper(macro, 0, __VA_ARGS__)))

#define _for_each_again() _for_each_helper
#define _for_each_helper(macro, i, a1, ...) \
  macro(i, a1)                              \
  __VA_OPT__(_for_each_again _parens (macro, (i) + 1, __VA_ARGS__))


#define _for_each_arg_again() _for_each_arg_helper
#define _for_each_arg_helper(macro, arg, i, a1, ...) \
  macro(arg, i, a1)                                  \
  __VA_OPT__(_for_each_arg_again _parens (macro, arg, (i) + 1, __VA_ARGS__))


#define _for_each_fold_again() _for_each_fold_helper
#define _for_each_fold_helper(macro, sep, i, a1, ...) \
  macro(i, a1)                                       \
  __VA_OPT__(sep _for_each_fold_again _parens (macro, sep, (i) + 1, __VA_ARGS__))


#define _for_each_cma_again() _for_each_cma_helper
#define _for_each_cma_helper(macro, i, a1, ...) \
  macro(i, a1)                                  \
  __VA_OPT__(, _for_each_cma_again _parens (macro, (i) + 1, __VA_ARGS__))


#define _parens ()

#else

#define FOR_EACH(macro, ...)            _expand(_for_each1(macro, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FOR_EACH_ARG(macro, arg, ...)   _expand(_for_each_arg1(macro, arg, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FOR_EACH_FOLD(macro, sep, ...)  _expand(_for_each_fold1(macro, sep, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FOR_EACH_COMMA(macro, ...)      _expand(_for_each_cma1(macro, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define _for_each_end(...)
#define _for_each_out
#define _for_each_cma ,

#define _for_each_get_end2()              0, _for_each_end
#define _for_each_get_end1(...)           _for_each_get_end2
#define _for_each_get_end(...)            _for_each_get_end1
#define _for_each_next0(test, next, ...)  next _for_each_out

#define _for_each_next1(test, next)             _for_each_next0(test, next, 0)
#define _for_each_next(test, next)              _for_each_next1(_for_each_get_end test, next)
#define _for_each0(f, i, x, peek, ...)          f(i, x) _for_each_next(peek, _for_each1)(f, (i) + 1, peek, __VA_ARGS__)
#define _for_each1(f, i, x, peek, ...)          f(i, x) _for_each_next(peek, _for_each0)(f, (i) + 1, peek, __VA_ARGS__)

#define _for_each_cma_next1(test, next)         _for_each_next0(test, _for_each_cma next, 0)
#define _for_each_cma_next(test, next)          _for_each_cma_next1(_for_each_get_end test, next)
#define _for_each_cma0(f, i, x, peek, ...)      f(i, x) _for_each_cma_next(peek, _for_each_cma1)(f, (i) + 1, peek, __VA_ARGS__)
#define _for_each_cma1(f, i, x, peek, ...)      f(i, x) _for_each_cma_next(peek, _for_each_cma0)(f, (i) + 1, peek, __VA_ARGS__)

#define _for_each_arg_next1(test, next)         _for_each_next0(test, next, 0)
#define _for_each_arg_next(test, next)          _for_each_arg_next1(_for_each_get_end test, next)
#define _for_each_arg0(f, a, i, x, peek, ...)   f(a, i, x) _for_each_arg_next(peek, _for_each_arg1)(f, a, (i) + 1, peek, __VA_ARGS__)
#define _for_each_arg1(f, a, i, x, peek, ...)   f(a, i, x) _for_each_arg_next(peek, _for_each_arg0)(f, a, (i) + 1, peek, __VA_ARGS__)

#define _for_each_fold_next1(test, s, next)     _for_each_next0(test, s next, 0)
#define _for_each_fold_next(test, s, next)      _for_each_fold_next1(_for_each_get_end test, s, next)
#define _for_each_fold0(f, s, i, x, peek, ...)  f(i, x) _for_each_fold_next(peek, s, _for_each_fold1)(f, s, (i) + 1, peek, __VA_ARGS__)
#define _for_each_fold1(f, s, i, x, peek, ...)  f(i, x) _for_each_fold_next(peek, s, _for_each_fold0)(f, s, (i) + 1, peek, __VA_ARGS__)

#endif
#define _expand(...) _expand4(_expand4(_expand4(_expand4(__VA_ARGS__))))
#define _expand4(...) _expand3(_expand3(_expand3(_expand3(__VA_ARGS__))))
#define _expand3(...) _expand2(_expand2(_expand2(_expand2(__VA_ARGS__))))
#define _expand2(...) _expand1(_expand1(_expand1(_expand1(__VA_ARGS__))))
#define _expand1(...) __VA_ARGS__


#endif // ZEN_MACROS_H
