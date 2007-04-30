#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lzsd.h"
#include "romfile.h"

/*
 * $ decompress <rom-0-file>
 */
int main(int argc, char *argv[])
{
	struct romfile *rom = get_romfile(argv[1], "autoexec.net");

	struct filedata *config = alloc_filedata(65536);
	config->size = lzs_unpack(rom->data + 0xC, rom->size - 0xC, config->data, config->size);

	char outname[64];
	strncpy(outname, argv[1], sizeof(outname));
	strcat(outname, ".decomp");

	put_filedata(outname, config);

	free(config);
	free(rom);
	return 0;
}
