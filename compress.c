#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lzsc.h"
#include "filedata.h"
#include "romfile.h"

/*
 * $ compress <rom-0-base> <plain-config>
 */
int main(int argc, char *argv[])
{
	struct romfile *rom = get_romfile(argv[1], "autoexec.net");

	struct filedata *config = get_filedata(argv[2]);
	rom->size = lzs_pack(config->data, config->size, rom->data + 0xC, 0x1000);
	rom->size += 0xC;

	char outname[64];
	strncpy(outname, argv[2], sizeof(outname));
	strcat(outname, ".comp");
	put_romfile(outname, rom);

	free(config);
	free(rom);
	return 0;
}
