#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "configdata.h"

struct cfg_patch {
	int code;
	int mask;
	int (* patch)(void *config, struct cfg_patch *patch, int code, const char *parameter);
	int offset;
	int size;
	int min;
	int max;
};

static int patch_8bit(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	int value = atoi(parameter);
	if (value < patch->min || value > patch->max)
		return -1;

	*((uint8_t *)(config + patch->offset)) = value;
	return 0;
}

static int patch_string(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	strncpy(config + patch->offset, parameter, patch->size);
	return 0;
}

static int patch_ip(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	return (inet_pton(AF_INET, parameter, config + patch->offset) <= 0) ? -1 : 0;
}

static struct cfg_patch patcharr[] = {{
	.code = CFG_LOCATION,
	.patch = patch_string,
	.offset = 0x0034,
	.size = 0x1d,
}, {
	.code = CFG_SYSNAME,
	.patch = patch_string,
	.offset = 0x0054,
	.size = 0x1d,
}, {
	.code = CFG_CONTACT,
	.patch = patch_string,
	.offset = 0x0074,
	.size = 0x1d,
}, {
	.code = CFG_DEFAULTGW,
	.patch = patch_ip,
	.offset = 0x10fc,
}, {
	.code = CFG_IPADDRESS,
	.patch = patch_ip,
	.offset = 0x2df8,
}, {
	.code = CFG_NETMASK,
	.patch = patch_8bit,
	.offset = 0x2dfe,
	.min = 0x00,
	.max = 0x20,
}, {
	.code = CFG_NAMESERVER,
	.patch = patch_ip,
	.offset = 0x32dc,
}};

int config_patch(void *config, int code, const char *parameter)
{
	int i;
	for (i = 0; i < (sizeof(patcharr) / sizeof(struct cfg_patch)); i++) {
		int mask = (patcharr[i].mask != 0) ? patcharr[i].mask : ~0;

		if ((patcharr[i].code & mask) == code)
			return patcharr[i].patch(config, &patcharr[i], code, parameter);
	}
	return -1;
}
