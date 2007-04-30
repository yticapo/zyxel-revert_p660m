#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "filedata.h"
#include "romfile.h"

struct romfile_header {
	uint16_t version;
	uint16_t size;
	uint16_t offset;

	char name[14];
} __attribute__((packed));

struct romfile * get_romfile(const char *filename, const char *romfile_name)
{
	struct romfile *romfile = malloc(sizeof(struct romfile));
	if (romfile == NULL)
		return NULL;

	romfile->file = get_filedata(filename);
	if (romfile->file == NULL) {
		free(romfile);
		return NULL;
	}

	romfile->baseoffset = 0x2000;

	struct romfile_header header;
	while (1) {
		memcpy(&header, (void *)(romfile->file->data) + romfile->baseoffset, sizeof(header));

		if (strcmp(header.name, romfile_name) == 0) {
			romfile->data = (void *)(romfile->file->data) + 0x2000 + htons(header.offset);
			romfile->size = htons(header.size);
			return romfile;
		}

		if (header.name[0] == 0 || header.name[0] == -1) {
			free(romfile->file);
			free(romfile);
			return NULL;
		}

		romfile->baseoffset += sizeof(header);
	}
}

int put_romfile(const char *filename, struct romfile *romfile)
{
	struct romfile_header header;
	memcpy(&header, (void *)(romfile->file->data) + romfile->baseoffset, sizeof(header));

	header.size = htons(romfile->size);
	memcpy((void *)(romfile->file->data) + romfile->baseoffset, &header, sizeof(header));

	return put_filedata(filename, romfile->file);
}
