#include <stdarg.h>
#include <stdio.h>

#define LOG_BUFFER_MAX (256)
char _buffer[LOG_BUFFER_MAX];

void logit(const char *file, const int line, const char *str, ...)
{
  va_list args;
  va_start(args, str);
  vsnprintf(_buffer, LOG_BUFFER_MAX, str, args);
  printf("\n%24s:%03d  %s", file, line, _buffer);
}

void qlogit(const char *str, ...)
{
 va_list args;
  va_start(args, str);
  vsnprintf(_buffer, LOG_BUFFER_MAX, str, args);
  printf("%s", _buffer);
}
