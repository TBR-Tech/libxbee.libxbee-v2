#ifndef __XBEE_H
#define __XBEE_H

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
#include <stdarg.h>

struct xbee;

struct xbee_pkt {
	unsigned char status;
	unsigned char options;
	unsigned char rssi;

	int datalen;
	unsigned char data[1];
};

/* --- xbee.c --- */
int xbee_setup(struct xbee **retXbee);

/* --- mode.c --- */
char **xbee_getModes(void);
char *xbee_getMode(struct xbee *xbee);
int xbee_setMode(struct xbee *xbee, char *name);

/* --- conn.c --- */
int xbee_conTypeIdFromName(struct xbee *xbee, char *name, unsigned char *id);

/* --- log.c --- */
void xbee_logSetTarget(FILE *f);

#endif /* __XBEE_H */

