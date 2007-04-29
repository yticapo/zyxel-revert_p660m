#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "lzsc.h"
#include "filedata.h"

struct rom0file {
	uint16_t version;
	uint16_t size;
	uint16_t offset;

	char name[14];
} __attribute__((packed));

int parse_rom0(struct filedata *filedata, const char *infile)
{
	struct rom0file file;

	/* zweite page enthält config */
	uint32_t offset = 0x2000;
	while (1) {
		memcpy(&file, (void *)(filedata->data) + offset, sizeof(file));

		if (strcmp(file.name, "autoexec.net") == 0) {
			printf("found autoexec.net: 0x%04x - 0x%04x (%d bytes)\n",
				0x2000 + htons(file.offset), 0x2000 + htons(file.offset) + htons(file.size), htons(file.size));

			/* 16 byte header */
			void *buf = (void *)(filedata->data) + 0x2000 + htons(file.offset) +12;

			/* little cleanup */
			memset(buf, 0, htons(file.size));

			struct filedata *indata = get_filedata(infile);
			int size = lzs_pack(indata->data, indata->size, buf, 0xC00);

			file.size = htons(size +12);

			memcpy((void *)(filedata->data) + offset, &file, sizeof(file));
			printf("new autoexec.net: 0x%04x - 0x%04x (%d bytes)\n",
				0x2000 + htons(file.offset), 0x2000 + htons(file.offset) + htons(file.size), htons(file.size));

			put_filedata("350LI2C1.rom.own", filedata);

			free(indata);
			return 0;
		}

		if (file.name[0] == 0 || file.name[0] == -1)
			return -1;

		offset += sizeof(file);
	}
}

int main(int argc, char *argv[])
{
	struct filedata *rom0 = get_filedata("350LI2C1.rom");
	parse_rom0(rom0, "350LI2C1.rom.own_decomp");

	free(rom0);
	return 0;
}
