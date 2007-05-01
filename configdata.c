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

static int patch_16bit(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	int value = atoi(parameter);
	if (value < patch->min || value > patch->max)
		return -1;

	*((uint16_t *)(config + patch->offset)) = htons(value);
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

static int patch_defaultgw(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	/* metric? */
	*((uint8_t *)(config + 0x110d)) = 0x02;
	return patch_ip(config, patch, code, parameter);
}

static int patch_portname(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	int num = code - patch->code;
	if (num < patch->min || num > patch->max)
		return -1;

	strncpy(config + patch->offset + (num -1) * 0x70, parameter, patch->size);
	return 0;
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
	.patch = patch_defaultgw,
	.offset = 0x10fc,
}, {
	.code = CFG_IPADDRESS,
	.patch = patch_ip,
	.offset = 0x2df8,
}, {
	.code = CFG_NETMASK,
	.patch = patch_8bit,
	.offset = 0x2dfe,
	.min = 0, .max = 32,
}, {
	.code = CFG_NAMESERVER,
	.patch = patch_ip,
	.offset = 0x32dc,
}, {
	.code = CFG_MACAGEING,
	.patch = patch_16bit,
	.offset = 0x53e8,
	.min = 0, .max = 65536,
}, {
	.code = CFG_PORTNAME_MASK,
	.mask = 0xFFFFFFE0,
	.patch = patch_portname,
	.offset = 0x336c,
	.size = 9,
	.min = 1, .max = 26,
}};

int config_patch(void *config, int code, const char *parameter)
{
	int i;
	for (i = 0; i < (sizeof(patcharr) / sizeof(struct cfg_patch)); i++) {
		int mask = (patcharr[i].mask != 0) ? patcharr[i].mask : ~0;

		if ((code & mask) == patcharr[i].code)
			return patcharr[i].patch(config, &patcharr[i], code, parameter);
	}
	return -1;
}
