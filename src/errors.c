#include "../include/errors.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Our global error handling function
// TODO: Improve it and make it output the line and where exactly the error
// comes from

NORETURN void throw_interpreter_error(const char *format, ...) {
  va_list args;
  va_start(args, format);

  vfprintf(stderr, format, args);

  va_end(args);
  exit(EXIT_FAILURE);
}

NORETURN void throw_parser_error(const char *format, ...) {
  va_list args;
  va_start(args, format);

  vfprintf(stderr, format, args);

  va_end(args);
  exit(EXIT_FAILURE);
}
