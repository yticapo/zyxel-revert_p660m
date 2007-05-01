#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "list.h"

#define HASH_BUCKETS 127

#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct lzs_state {
	uint8_t *srcblkstart;
	uint8_t *src;
	uint32_t srcsize;

	uint8_t *dstblkstart;
	uint8_t *dst;
	uint32_t dstsize;

	uint32_t bitbuf;
	uint32_t bitcnt;

	struct list_head *hash;
};

struct lzs_hash_entry {
	struct list_head list;
	uint8_t *pos;
};

/* TODO: check dstsize */
static int put_bits(struct lzs_state *state, uint32_t bits, uint32_t len)
{
	state->bitbuf <<= len;
	state->bitbuf |= bits;
	state->bitcnt += len;

	while (state->bitcnt >= 8) {
		state->bitcnt -= 8;
		*(state->dst)++ = (state->bitbuf >> (state->bitcnt)) & 0xFF;
//		printf("    wrote byte: 0x%02x\n", *(state->dst -1));
	}

	return 0;
}

/* TODO: check dstsize */
static int put_literal_byte(struct lzs_state *state, uint8_t byte)
{
//	printf("    put_literal_byte: 0x%02x\n", byte);
	return put_bits(state, (0 << 8) | byte, 1+8);
}

/* TODO: check dstsize */
static int put_compressed_string(struct lzs_state *state, uint32_t offset, uint32_t len)
{
//	printf("    put_compressed_string: offset=0x%03x len=0x%03x\n", offset, len);

	if (offset > 0x7ff || len > 0x800)
		printf("      ERROR\n");

	if (offset < 128)
		put_bits(state, (1 << 8) | (1 << 7) | offset, 1+1+7);
	else
		put_bits(state, (1 << 12) | (0 << 11) | offset, 1+1+11);

	if (len <= 4) {
		/*
		 * 00 - 2
		 * 01 - 3
		 * 10 - 4
		 */
		put_bits(state, len - 2, 2);

	} else if (len < 8) {
		/*
		 * 1100 - 5
		 * 1101 - 6
		 * 1110 - 7
		 */
		put_bits(state, len + 7, 4);

	} else if (len >= 8) {
		/*
		 * 1111 0000 - 8
		 * 1111 0001 - 9
		 * ...
		 * 1111 1110 - 22
		 * 1111 1111 0000 - 23
		 * 1111 1111 0001 - 24
		 * ...
		 */
		len -= 8;
		put_bits(state, 15, 4);
		while (len >= 15) {
			put_bits(state, 15, 4);
			len -= 15;
		}
		put_bits(state, len, 4);
	}
	return 0;
}

/* TODO: check dstsize */
static int put_blockend(struct lzs_state *state)
{
	/* 7bit offset = 0 -> end code */
	put_bits(state, (1 << 8) | (1 << 7) | 0, 1+1+7);

	/* align to bytes */
	if (state->bitcnt)
		put_bits(state, 0, 8 - state->bitcnt);

//	printf(" =============== BLOCK END =============== \n");
	return 0;
}

static int put_zyxel_header(struct lzs_state *state)
{
	uint16_t len = state->src - state->srcblkstart;
	uint16_t lenc = state->dst - state->dstblkstart;

	/* remove own header size */
	lenc -= 4;

//	printf("header of previous block: 0x%04x%04x\n", len, lenc);

	uint8_t *p = state->dstblkstart;
	p[0] = (len >> 8) & 0xFF;
	p[1] = len & 0xFF;

	p[2] = (lenc >> 8) & 0xFF;
	p[3] = lenc & 0xFF;

	return 0;
}

static int alloc_hash(struct lzs_state *state)
{
	state->hash = malloc(sizeof(struct lzs_hash_entry) * HASH_BUCKETS);
	if (state->hash == NULL) {
		perror("alloc_hashtable(): malloc()");
		return -1;
	}

	int i;
	for (i = 0; i < HASH_BUCKETS; i++)
		INIT_LIST_HEAD(&state->hash[i]);

	return 0;
}

static int free_hash(struct lzs_state *state)
{
	int i;
	for (i = 0; i < HASH_BUCKETS; i++) {
		struct lzs_hash_entry *entry, *tmp;
		list_for_each_entry_safe(entry, tmp, &state->hash[i], list) {
			list_del(&entry->list);
			free(entry);
		}
	}

	return 0;
}

static int hash_key_calc(uint8_t *data)
{
	int key = 0x456789AB;

	key = (key << 5) ^ (key >> 27) ^ data[0];
	key = (key << 5) ^ (key >> 27) ^ data[1];

	return key % HASH_BUCKETS;
}

static int hash_add(struct lzs_state *state, uint8_t *data)
{
	struct lzs_hash_entry *entry = malloc(sizeof(struct lzs_hash_entry));
	if (entry == NULL) {
		perror("hash_add_bytes(): malloc()");
		return -1;
	}

	entry->pos = data;

	list_add(&entry->list, &state->hash[hash_key_calc(data)]);
	return 0;
}

static int getMatchLen(uint8_t *a, uint8_t *b, uint32_t maxlen)
{
	/* shortcut, first 2 bytes *must* match */
	if ((a[0] ^ b[0]) | (a[1] ^ b[1]))
		return 0;

	a += 2;
	b += 2;
	maxlen -= 2;

	int retval = 2;
	while ((*a++ == *b++) && maxlen--)
		retval++;

	return retval;
}

int lzs_pack(uint8_t *srcbuf, int srcsize, uint8_t *dstbuf, int dstsize)
{
	struct lzs_state state = {
		.srcblkstart = srcbuf,
		.src = srcbuf,
		.srcsize = srcsize,

		.dstblkstart = dstbuf,
		.dst = dstbuf,
		.dstsize = dstsize,

		.bitbuf = 0,
		.bitcnt = 0,
	};

	alloc_hash(&state);

	/* at least 2 bytes in input */
	while (state.src +2 <= srcbuf + srcsize) {

		/* new dst block: insert dummy header */
		if (state.dstblkstart == state.dst)
			state.dst += 4;

		int key = hash_key_calc(state.src);
		int maxlen = MIN(state.srcblkstart + 2048, srcbuf + srcsize) - state.src;

//		printf("searching for 0x%02x%02x abs=0x%04x key=0x%02x maxlen=%d\n",
//			state.src[0], state.src[1], state.src - srcbuf, key, maxlen);

		int bestmatchlen = 0;
		struct lzs_hash_entry *search, *tmp, *bestmatch = NULL;
		list_for_each_entry_safe(search, tmp, &state.hash[key], list) {
			/* hash entry too old, discard it */
			if (search->pos + 2048 <= state.src) {
				list_del(&search->list);
				free(search);
				continue;
			}

			/* get length of match (min. 2, 0 if collision) */
			int matchlen = getMatchLen(search->pos, state.src, maxlen);
//			printf("  testing pos=0x%03x has 0x%02x matching bytes\n", (state.src - search->pos), matchlen);

			if (matchlen > bestmatchlen) {
				bestmatchlen = matchlen;
				bestmatch = search;
			}
		}

		/* found something? */
		if (bestmatch != NULL) {
//			printf("  selected pos=0x%03x with 0x%02x matching bytes\n", (state.src - bestmatch->pos), bestmatchlen);
			put_compressed_string(&state, state.src - bestmatch->pos, bestmatchlen);
			/* add bytes to history hash */
			while (bestmatchlen--)
				hash_add(&state, state.src++);

		} else {
			put_literal_byte(&state, *state.src);
			hash_add(&state, state.src++);
		}

		/* block full? */
		if (state.src - state.srcblkstart >= 2048) {
			put_blockend(&state);

			put_zyxel_header(&state);
			state.srcblkstart = state.src;
			state.dstblkstart = state.dst;
		}
	}

	/* add remaining bytes (== last one) as literal */
	if (state.src < srcbuf + srcsize)
		put_literal_byte(&state, *state.src++);

	put_blockend(&state);
	put_zyxel_header(&state);

	free_hash(&state);

	printf("lzs_pack: packed %d (%d) bytes to %d (%d) bytes\n",
		state.src - srcbuf, srcsize, state.dst - dstbuf, dstsize);

	return state.dst - dstbuf;
}
