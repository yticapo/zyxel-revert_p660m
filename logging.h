#ifndef _LOGGING_H_
#define _LOGGING_H_

#define LOG_DEBUG 5
#define LOG_INFO  4
#define LOG_NOTICE 3
#define LOG_WARN 2
#define LOG_ERROR 1
#define LOG_CRIT 0

#define LOG_EVERYTIME 0

int log_init(const char *logfile);
void log_close(void);
void log_setprio(int prio);

int log_print(int prio, const char *fmt, ... );

#endif /* _LOGGING_H_ */
