/*
  libxbee - a C library to aid the use of Digi's XBee wireless modules
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
#include <stdlib.h>

#include "internal.h"
#include "tx.h"
#include "io.h"
#include "log.h"

int xbee_txSerialXBee(struct xbee *xbee, struct bufData *buf) {
	unsigned char chksum;
	int i;
		
	chksum = 0;
	
	xbee_io_writeRawByte(xbee, 0x7E);
	xbee_io_writeEscapedByte(xbee, ((buf->len >> 8) & 0xFF));
	xbee_io_writeEscapedByte(xbee, ( buf->len       & 0xFF));
	
	for (i = 0; i < buf->len; i++) {
		chksum += buf->buf[i];
		xbee_io_writeEscapedByte(xbee, buf->buf[i]);
	}
	
	xbee_io_writeEscapedByte(xbee, 0xFF - chksum);
	
	return 0;
}

int _xbee_tx(struct xbee *xbee) {
	int ret;
	struct bufData *buf;
	
	if (!xbee) return XBEE_ENOXBEE;
	
	while (xbee->running) {
		buf = ll_ext_head(&xbee->txList);
		if (!buf) {
			xsys_sem_wait(&xbee->txSem);
			continue;
		} else {
#ifdef XBEE_NO_RTSCTS
			/* it appears that the XBee's are unable to keep up if we just go full tilt! */
			usleep(10000);
#endif
		}
		
		if (!xbee->f->tx) {
			xbee_log(-999,"xbee->f->tx(): not registered!");
			return XBEE_EINVAL;
		}
		
		if ((ret = xbee->f->tx(xbee, buf)) != 0) {
			xbee_log(1,"xbee->f->tx(): returned %d", ret);
		}
		
		free(buf);
	}
	
	return 0;
}

int xbee_tx(struct xbee *xbee) {
	int ret;
	if (!xbee) return 1;
	
	xbee->txRunning = 1;
	while (xbee->running) {
		ret = _xbee_tx(xbee);
		xbee_log(1,"_xbee_tx() returned %d\n", ret);
		if (!xbee->running) break;
		usleep(XBEE_TX_RESTART_DELAY * 1000);
	}
	xbee->txRunning = 0;
	
	return 0;
}
