/***************************************************************************
 *   Copyright (C) 06/2006 by Olaf Rempel                                  *
 *   razzor@kopf-tisch.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "logging.h"

#define BUFSIZE 8192

static FILE *log_fd = NULL;
static int log_prio = LOG_EVERYTIME;
static char *buffer = NULL;

int log_print(int prio, const char *fmt, ...)
{
	va_list az;
	int len = 0, retval;

	if (prio < log_prio)
		return 0;

	if (buffer == NULL) {
		buffer = malloc(BUFSIZE);
		if (buffer == NULL) {
			fprintf(stderr, "log_print(): out of memory\n");
			return -1;
		}
	}

	if (log_fd != NULL) {
		time_t  tzgr;
		time(&tzgr);

		len += strftime(buffer, BUFSIZE, "%b %d %H:%M:%S :", localtime(&tzgr));
	}

	va_start(az, fmt);
	len += vsnprintf(buffer + len, BUFSIZE - len, fmt, az);
	va_end(az);

	if (len < 0 || len >= BUFSIZE) {
		errno = 0;
		return log_print(LOG_ERROR, "log_print: arguments too long");
	}

	if (errno) {
		len += snprintf(buffer + len, BUFSIZE - len, ": %s", strerror(errno));
		errno = 0;
	}

	retval = fprintf((log_fd ? log_fd : stderr), "%s\n", buffer);
	fflush(log_fd);
	return retval;
}

void log_close(void)
{
	if (buffer)
		free(buffer);

	if (log_fd)
		fclose(log_fd);
}

int log_init(const char *logfile)
{
	if (log_fd != NULL)
		log_close();

	log_fd = fopen(logfile, "a");
	if (log_fd == NULL) {
		fprintf(stderr, "log_init(): can not open logfile");
		return -1;
	}
	return 0;
}

void log_setprio(int prio)
{
	log_prio = prio;
}
