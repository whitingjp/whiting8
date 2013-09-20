#include <stdarg.h>
#include <stdio.h>

#define LOG_BUFFER_MAX (256)
char _buffer[LOG_BUFFER_MAX];
const char* log_file = NULL;

void logit(const char *file, const int line, const char *str, ...)
{
  va_list args;
  va_start(args, str);
  vsnprintf(_buffer, LOG_BUFFER_MAX, str, args);
  printf("\n%24s:%03d  %s", file, line, _buffer);
  if(log_file)
  {
    FILE* out = fopen(log_file, "a");
    fprintf(out, "\n%24s:%03d  %s", file, line, _buffer);
    fclose(out);
  }
}

void qlogit(const char *str, ...)
{
  va_list args;
  va_start(args, str);
  vsnprintf(_buffer, LOG_BUFFER_MAX, str, args);
  printf("%s", _buffer);
  if(log_file)
  {
    FILE* out = fopen(log_file, "a");
    fprintf(out, "%s", _buffer);
    fclose(out);
  }
}

void set_logfile(const char *file)
{
	log_file = file;
	FILE* out = fopen(log_file, "w");
    if (out == NULL)
      printf("\nFailed to open log_file");
  	fclose(out);
}