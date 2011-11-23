/*
  libxbee - a C library to aid the use of Digi's Series 1 XBee modules
            running in API mode (AP=2).

  Copyright (C) 2009  Attie Grande (attie@attie.co.uk)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "internal.h"
#include "errors.h"
#include "log.h"
#include "xsys.h"
#include "io.h"

int xbee_io_open(struct xbee *xbee) {
	int fd;
	FILE *f;
	xbee->device.ready = 0;
	
	/* open the device */
	if ((fd = xsys_open(xbee->device.path, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
		xbee_perror("xsys_open()");
		return XBEE_EOPENFAILED;
	}
	
	/* lock the device */
#warning TODO - lock the device
	
	/* open the device as a buffered FILE */
	if ((f = xsys_fdopen(fd, "r+")) == NULL) {
		xsys_close(fd);
		xbee_perror("xsys_fdopen()");
		return XBEE_EOPENFAILED;
	}
	
	/* flush the serial port */
	xsys_fflush(f);
	
	/* disable buffering */
	xsys_disableBuffer(f);
	
	/* setup serial port (baud, control lines etc...) */
	xsys_setSerial(fd, f, xbee->device.baudrate);
	
	xbee->device.fd = fd;
	xbee->device.f = f;
	xbee->device.ready = 1;
	
	return XBEE_ENONE;
}

void xbee_io_close(struct xbee *xbee) {
	int fd;
	FILE *f;
	xbee->device.ready = 0;
	
	fd = xbee->device.fd;
	xbee->device.fd = 0;
	f = xbee->device.f;
	xbee->device.f = NULL;
	
	xsys_fclose(f);
	xsys_close(fd);
}

int xbee_io_reopen(struct xbee *xbee) {
	xbee_io_close(xbee);
	usleep(10000);
	return xbee_io_open(xbee);
}

/* ######################################################################### */

int xbee_io_getRawByte(FILE *f, unsigned char *cOut) {
	unsigned char c;
	int ret = XBEE_EUNKNOWN;
	int retries = XBEE_IO_RETRIES;
	*cOut = 0;

	do {
		if ((ret = xsys_select(f, NULL)) == -1) {
			xbee_perror("xbee_select()");
			if (errno == EINTR) {
				ret = XBEE_ESELECTINTERRUPTED;
			} else {
				ret = XBEE_ESELECT;
			}
			goto done;
		}
	
		if (xsys_fread(&c, 1, 1, f) == 0) {
			/* for some reason nothing was read... */
			if (xsys_ferror(f)) {
				char *s;
				if (xsys_feof(f)) {
					xbee_logsstderr("EOF detected...");
					ret = XBEE_EEOF;
					goto done;
				}
				if (!(s = strerror(errno))) {
					xbee_logsstderr("Unknown error detected (%d)",errno);
				} else {
					xbee_logsstderr("Error detected (%s)",s);
				}
				usleep(1000);
			} else {
				/* no error? weird... try again */
				usleep(100);
			}
		} else {
			break;
		}
	} while (--retries);
	
	if (!retries) {
		ret = XBEE_EIORETRIES;
	} else {
		*cOut = c;
		ret = XBEE_ENONE;
	}
	
done:
	return ret;
}

int xbee_io_getEscapedByte(FILE *f, unsigned char *cOut) {
	unsigned char c;
	int ret = XBEE_EUNKNOWN;
	*cOut = 0;

	if ((ret = xbee_io_getRawByte(f, &c)) != 0) return ret;
	if (c == 0x7D) {
		if ((ret = xbee_io_getRawByte(f, &c)) != 0) return ret;
		c ^= 0x20;
	}
	*cOut = c;
	return ret;
}

/* ######################################################################### */

int xbee_io_writeRawByte(FILE *f, unsigned char c) {
	int ret = XBEE_EUNKNOWN;
	int retries = XBEE_IO_RETRIES;
	
	do {
		if (xsys_fwrite(&c, 1, 1, f) == 0) {
			/* for some reason nothing was written... */
			if (xsys_ferror(f)) {
				char *s;
				if (xsys_feof(f)) {
					xbee_logsstderr("EOF detected...");
					ret = XBEE_EEOF;
					goto done;
				}
				if (!(s = strerror(errno))) {
					xbee_logsstderr("Unknown error detected (%d)",errno);
				} else {
					xbee_logsstderr("Error detected (%s)",s);
				}
				usleep(1000);
			} else {
				/* no error? weird... try again */
				usleep(100);
			}
		}
	} while (--retries);
	
	if (!retries) {
		ret = XBEE_EIORETRIES;
	} else {
		ret = XBEE_ENONE;
	}
	
done:
	return ret;
}

int xbee_io_writeEscapedByte(FILE *f, unsigned char c) {
	int fd;
	fd = fileno(f);
	
	if (c == 0x7E ||
			c == 0x7D ||
			c == 0x11 ||
			c == 0x13) {
		if (xbee_io_writeRawByte(f, 0x7D)) return XBEE_EIO;
		c ^= 0x20;
	}
	if (xbee_io_writeRawByte(f, c)) return XBEE_EIO;
	return 0;
}