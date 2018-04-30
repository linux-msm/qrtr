#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#define QLOG_BUF_SIZE 512

static const char default_tag[] = "libqrtr";
static const char *current_tag = default_tag;

void qlog_setup(const char *tag)
{
	current_tag = tag;
}

static const char *get_priority_string(int priority)
{
	switch (priority) {
	case LOG_EMERG:
		return "EMERG";
	case LOG_ALERT:
		return "ALERT";
	case LOG_CRIT:
		return "CRIT";
	case LOG_ERR:
		return "ERROR";
	case LOG_WARNING:
		return "WARNING";
	case LOG_NOTICE:
		return "NOTICE";
	case LOG_INFO:
		return "INFO";
	case LOG_DEBUG:
		return "DEBUG";
	}
	return "";
}

void qlog(int priority, const char *format, ...)
{
	char buf[QLOG_BUF_SIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, QLOG_BUF_SIZE, format, ap);
	va_end(ap);

	fprintf(stderr, "%s %s: %s\n",
	        get_priority_string(priority), current_tag, buf);
}
