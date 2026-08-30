#ifndef FLEA_ERRORS_H
#define FLEA_ERRORS_H

#if defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NORETURN _Noreturn
#else
#define NORETURN
#endif

NORETURN void throw_interpreter_error(const char *format, ...);

NORETURN void throw_parser_error(const char *format, ...);

#endif
