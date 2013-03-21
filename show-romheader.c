#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

const char* prog_name = "show-romheader";
const char* prog_version_str = "0.1";

void
print_version ()
{
	printf ("%s version %s\n", prog_name, prog_version_str);
}

void
print_usage ()
{
	printf ("%s version %s\n"
			"Usage: \n"
			"      %s [options] <source file> <destination file>\n"
			"\n"
			"      General options\n"
			"        -h, --help      Show this help\n"
			"        -o, --offset    Set offset\n"
			"\n"
			"      Example\n"
			"           %s --offset=0x2000 spt.dat\n"
			, prog_name, prog_version_str
			, prog_name
			, prog_name);
}

typedef struct _block_object {
	uint8_t  name [15 + 1];
	uint32_t uncomp_size;
	uint32_t comp_size;
	uint32_t offset;
} Block_Object;

/**
 * Block
 *
 * A single Block within rom-0 file.  Consists of (a) header
 * information and (b) of an object vector holding all (sub) objects
 * within the block.
 */
typedef struct _block {
	uint8_t id;
	uint8_t magic_number;
	uint16_t obj_count;
	uint16_t size;
	Block_Object*  object_v;
} Block;

void print_object (Block_Object* obj)
{
	printf ("\tName: %s\n", obj->name);
	printf ("\tUncompressed size: 0x%04x\n", obj->uncomp_size);
	printf ("\tCompressed size:   0x%04x\n", obj->comp_size);
	printf ("\tOffset:  0x%04x\n", obj->offset);
}

void print_block_header (Block* b)
{
	printf ("Block #%d\n", b->id);
	printf ("\tMagic number %x\n", b->magic_number);
	printf ("\tObject count %d\n", b->obj_count);
	printf ("\tBlock size %x\n", b->size);
}

void print_block_obj (Block* b, int index)
{
	Block_Object* obj_p = &(b->object_v[index]);
	printf ("Object index #%d\n", index);
	print_object (obj_p);
}

void print_block_all_obj (Block* b)
{
	int i = 0;
	for (; i < b->obj_count; i++) {
		print_block_obj (b, i);
	}
}

void print_block (Block* b)
{
	print_block_header(b);
	print_block_all_obj (b);
}

void show_block_header (unsigned int offset, FILE* src_file)
{
	uint8_t* src_bytes;  // stores all read input bytes from src file
	Block b;
	b.object_v = NULL;
	int header_bytes_size = sizeof(b.id) + sizeof (b.magic_number)
		+ sizeof (b.obj_count) + sizeof (b.size);
	src_bytes = (uint8_t*) malloc (header_bytes_size);

	// first, read in block header
	fseek (src_file, offset, SEEK_SET);
	fread (src_bytes, 1, header_bytes_size, src_file);
	b.id = src_bytes[0];
	b.magic_number = src_bytes[1];
	b.obj_count = (src_bytes[2] << 8) | src_bytes[3];
	b.size = (src_bytes[4] << 8) | src_bytes[5];

	print_block_header (&b);
	
	// second, make some room
	src_bytes = (uint8_t*) realloc (src_bytes, b.size - header_bytes_size);
	b.object_v = (Block_Object*) malloc (sizeof(Block_Object) * b.obj_count);

	// third, read all objects within block
	fread (src_bytes, 1, b.size - header_bytes_size, src_file);
	int i = 0;
	uint8_t* p = src_bytes;
	for (; i < b.obj_count; i++) {
		// read name
		int j = 0;
		for (; j < 14; j++) {
			b.object_v[i].name[j] = *p;
			++p;
		}
		b.object_v[i].name[15] = '\0';
		
		uint8_t* pp =  p;

		// read sizes
		b.object_v [i].uncomp_size  = (*pp << 8);
		++pp;
		b.object_v [i].uncomp_size |= *pp;
		++pp;
		b.object_v [i].comp_size  = *pp << 8;
		++pp;
		b.object_v [i].comp_size |= *pp;
		++pp;

		// read offset
		b.object_v [i].offset  = *pp << 8;
		++pp;
		b.object_v [i].offset |= *pp;
		++pp;

		p = (uint8_t*) pp;
	}

	print_block_all_obj (&b);
	free (src_bytes);
}

int main(int argc, char** argv) {
	int c;
	FILE* src_file = NULL;
	unsigned int offset   = 0x2000;

	static struct option long_options[] =
		{
			{"help",    no_argument,       0, 'h'},
			{"version",    no_argument,    0, 'v'},
			{"offset",  required_argument, 0, 'o'},
			{0, 0, 0, 0}
		};
	int option_index = 0;

	while (1) {
		c = getopt_long (argc, argv, "vho:", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_usage();
			return 0;
		case 'o':
			offset = strtol (optarg, (char **) NULL, 16);
			printf ("Debug: offset %i\n", offset);
			break;
		case 'v':
			print_version();
			return 0;
		default:
			break;
		}
	}

	// check if one arguments are provided
	if ((optind + 1) > argc) {
		return 1;
	}
	printf ("Opening source file: %s\n", argv[optind]);
	src_file = fopen (argv[optind], "rb");
	if (!src_file) {
		return 2;
	}

	show_block_header (offset, src_file);
	fclose (src_file);

	return 0;
}
