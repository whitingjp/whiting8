void logit(const char *file, const int line, const char *str, ...);
void qlogit(const char *str, ...);
void set_logfile(const char *file);
#define LOG(...) logit(__FILE__, __LINE__, __VA_ARGS__)
#define QLOG(...) qlogit(__VA_ARGS__)