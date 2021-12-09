#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

void error_exit(bool se, const char *format, ...)
{
	int e = errno;
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	if (se)
		fprintf(stderr, "\nerrno: %d (%s)\n", e, strerror(e));

	exit(EXIT_FAILURE);
}
