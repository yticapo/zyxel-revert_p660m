#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "lzsd.h"
#include "filedata.h"

struct rom0file {
	uint16_t version;
	uint16_t size;
	uint16_t offset;

	char name[14];
} __attribute__((packed));

int parse_rom0(struct filedata *filedata, const char *outfile)
{
	struct rom0file file;

	/* zweite page enthält config */
	uint32_t offset = 0x2000;
	while (1) {
		memcpy(&file, (void *)(filedata->data) + offset, sizeof(file));

		if (strcmp(file.name, "autoexec.net") == 0) {
			printf("found autoexec.net: 0x%04x - 0x%04x (%d bytes)\n",
				0x2000 + htons(file.offset), 0x2000 + htons(file.offset) + htons(file.size), htons(file.size));

			/* 64kb sollten reichen */
			struct filedata *out = alloc_filedata(65535);

			/* 16 byte header */
			void *buf = (void *)(filedata->data) + 0x2000 + htons(file.offset) +12;

			out->size = lzs_unpack(buf, htons(file.size) -12, out->data, out->size);
			put_filedata(outfile, out);
			free(out);

			return 0;
		}

		if (file.name[0] == 0 || file.name[0] == -1)
			return -1;

		offset += sizeof(file);
	}
}

int main(int argc, char *argv[])
{
	struct filedata *in = get_filedata(argv[1]);

	char outname[64];
	strncpy(outname, argv[1], sizeof(outname));
	strcat(outname, ".own_decomp");

	parse_rom0(in, outname);

	free(in);
	return 0;
}
