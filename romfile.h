#ifndef _ROMFILE_H_
#define _ROMFILE_H_

#include <stdint.h>

#include "filedata.h"

struct romfile {
	struct filedata *file;

	/* offset to struct rom0file-struct in file->data */
	uint32_t baseoffset;

	/* the data */
	uint8_t *data;
	uint16_t size;
};

struct romfile * get_romfile(const char *filename, const char *romfile_name);
int put_romfile(const char *filename, struct romfile *romfile);

#endif /* _ROMFILE_H_ */
