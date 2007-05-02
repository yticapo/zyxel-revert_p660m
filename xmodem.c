#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "context.h"
#include "filedata.h"
#include "logging.h"

enum {
	XM_SOH = 0x01,
	XM_EOT = 0x04,
	XM_ACK = 0x06,
	XM_NACK = 0x15,
	XM_C = 0x43
};

struct xmodem_pkt {
	uint8_t header;
	uint8_t count;
	uint8_t ncount;
	uint8_t data[128];
	uint16_t crc;
} __attribute__((packed));


static void calc_crc(struct xmodem_pkt *pkt)
{
	uint16_t crc = 0;
	uint8_t *ptr = pkt->data;

	int i, j;
	for (i = 0; i < sizeof(pkt->data); i++) {
		crc = crc ^ (*ptr++ << 8);

		for (j = 0; j < 8; j++)
			crc = (crc & 0x8000) ? (crc << 1 ^ 0x1021) : (crc << 1);
	}

	/* byteswap */
	pkt->crc = ((crc & 0xFF00) >> 8) | ((crc & 0xFF) << 8);
}

int xmodem_read(int fd, void *privdata)
{
	struct context *ctx = (struct context *)privdata;

	struct xmodem_pkt pkt;

	int len = read(fd, &pkt, sizeof(pkt));
	if (len <= 0) {
		log_print(LOG_WARN, "xmodem_read(): read()");
		return -1;
	}

	int pktnum = 0;

	switch (pkt.header) {
	case XM_C:	/* first packet */
		log_print(LOG_DEBUG, "%s: XMODEM started (%d bytes)", ctx->devname, ctx->file->size);
		pktnum = 0;
		break;

	case XM_ACK:	/* next packet */
		if (ctx->lastpkt * 128 >= ctx->file->size)
			return -1;

		pktnum = ctx->lastpkt +1;
		break;

	case XM_NACK:	/* resend last packet */
		pktnum = ctx->lastpkt;
		break;
	}

	if (pktnum * 128 < ctx->file->size) {
		pkt.header = XM_SOH;
		pkt.count = ((pktnum +1) & 0xFF);
		pkt.ncount = 0xFF - pkt.count;

		memcpy(pkt.data, (void *)(ctx->file->data) + pktnum * 128, 128);

		calc_crc(&pkt);
		write(fd, &pkt, sizeof(pkt));

	} else {
		log_print(LOG_DEBUG, "%s: XMODEM completed", ctx->devname);
		pkt.header = XM_EOT;
		write(fd, &pkt, 1);
	}

	ctx->lastpkt = pktnum;
	return 0;
}
