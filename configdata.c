#include <stdio.h>
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
	/* defaultgw has metric of 2 */
	*((uint8_t *)(config + 0x110d)) = 0x02;
	return patch_ip(config, patch, code, parameter);
}

static int patch_portenable(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	int port = code - patch->code;
	if (port < 1 || port > 26)
		return -1;

	int status = (strcmp(parameter, "on") == 0);
	*((uint8_t *)(config + 0x336b + (port -1) * 0x70)) = (status ? 0x03 : 0x01);
	return 0;
}

static int patch_portname(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	int port = code - patch->code;
	if (port < 1 || port > 26)
		return -1;

	strncpy(config + 0x336c + (port -1) * 0x70, parameter, patch->size);
	return 0;
}

static int patch_snmp_community(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	strncpy(config + 0x00A0 + patch->offset, parameter, patch->size);
	*((uint8_t *)(config + 0x0101)) = 0x01;

	strncpy(config + 0x9914 + patch->offset, parameter, patch->size);
	*((uint8_t *)(config + 0x9975)) = 0x01;

	strncpy(config + 0x9984 + patch->offset, parameter, patch->size);
	*((uint8_t *)(config + 0x99e5)) = 0x01;

	strncpy(config + 0x99f4 + patch->offset, parameter, patch->size);
	*((uint8_t *)(config + 0x9a55)) = 0x01;

	return 0;
}

static int patch_portvlan(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	int port = code - patch->code;
	if (port < 1 || port > 26)
		return -1;

	char *tmp, *buf = strdup(parameter);
	int vlanid = atoi(strtok_r(buf, ":", &tmp));
	int check_ingress = (strcmp(strtok_r(NULL, ":", &tmp), "check") == 0);
	int accept_all = (strcmp(strtok_r(NULL, ":", &tmp), "all") == 0);
	free(buf);

	if (vlanid < 0 || vlanid > 255)
		return -1;

	*((uint8_t *)(config + 0x5eff + (port -1) * 0x30)) = vlanid;
	*((uint8_t *)(config + 0x5f00 + (port -1) * 0x30)) = accept_all ? 0x00 : 0x01;
	*((uint8_t *)(config + 0x5f01 + (port -1) * 0x30)) = check_ingress ? 0x01 : 0x00;

	return 0;
}

/* TODO: not working "unknown ctl bits" */
static int patch_dot1qvlan(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	return -1;

	int id = code - patch->code;
	if (id < 1 || id > 32)
		return -1;

	char *tmp, *buf = strdup(parameter);
	int vlanid = atoi(strtok_r(buf, ":", &tmp));
	char *name = strtok_r(NULL, ":", &tmp);

	strncpy(config + 0x63e8 + (id -1) * 0xd0, name, 12);
	*((uint16_t *)(config + 0x64b4 + (id -1) * 0xd0)) = htons(vlanid);
	*((uint8_t *)(config + 0x64b6 + (id -1) * 0xd0)) = 0x03;

	/* all ports forbidden & not tagging as default */
	*((uint32_t *)(config + 0x63f4 + (id -1) * 0xd0)) = 0x03FFFFFF;
	*((uint32_t *)(config + 0x6434 + (id -1) * 0xd0)) = 0x03FFFFFF;
	*((uint32_t *)(config + 0x6474 + (id -1) * 0xd0)) = 0;

	free(buf);
	return 0;
}

/* TODO: not working "unknown ctl bits" */
static int patch_dot1qport(void *config, struct cfg_patch *patch, int code, const char *parameter)
{
	return -1;

	int id = code - patch->code;
	if (id < 1 || id > 32)
		return -1;

	char *tmp, *buf = strdup(parameter);
	int port = atoi(strtok_r(buf, ":", &tmp));
	char *control = strtok_r(NULL, ":", &tmp);
	char *tagging = strtok_r(NULL, ":", &tmp);

	if (port < 1 || port > 26) {
		free(buf);
		return -1;
	}

	uint32_t *a = (uint32_t *)(config + 0x63f4 + (id -1) * 0xd0);
	uint32_t *b = (uint32_t *)(config + 0x6434 + (id -1) * 0xd0);
	uint32_t *c = (uint32_t *)(config + 0x6474 + (id -1) * 0xd0);

	uint32_t value = (1 << (port -1));
	if (strcmp(control, "normal") == 0) {
		*a &= ~value;
		*b &= ~value;

	} else if (strcmp(control, "fixed") == 0) {
		*a &= ~value;
		*b |= value;

	} else if (strcmp(control, "forbidden") == 0) {
		*a |= value;
		*b |= value;

	} else {
		free(buf);
		return -1;
	}

	if (strcmp(tagging, "tagging") == 0)
		*c |= value;
	else
		*c &= ~value;

	free(buf);
	return 0;
}

static struct cfg_patch patcharr[] = {{
	.code = CFG_PASSWORD,
	.patch = patch_string,
	.offset = 0x0014,
	.size = 0x1d,
}, {
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
	.code = CFG_SNMP_READ_COMMUNITY,
	.patch = patch_snmp_community,
	.offset = 0x0000,
	.size = 0x1f,
}, {
	.code = CFG_SNMP_WRITE_COMMUNITY,
	.patch = patch_snmp_community,
	.offset = 0x0020,
	.size = 0x1f,
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
	.code = CFG_CPUVLANID,
	.patch = patch_8bit,
	.offset = 0x5d69,
	.min = 1, .max = 255,
}, {
	.code = CFG_PORTENABLE_MASK,
	.mask = 0xFFFFFFE0,
	.patch = patch_portenable,
}, {
	.code = CFG_PORTNAME_MASK,
	.mask = 0xFFFFFFE0,
	.patch = patch_portname,
	.size = 0x9,
}, {
	.code = CFG_PORTVLAN_MASK,
	.mask = 0xFFFFFFE0,
	.patch = patch_portvlan,
}, {
	.code = CFG_DOT1QVLAN_MASK,
	.mask = 0xFFFFFFE0,
	.patch = patch_dot1qvlan,
}, {
	.code = CFG_DOT1QPORT_MASK,
	.mask = 0xFFFFFFE0,
	.patch = patch_dot1qport,
}};

int config_patch(void *config, int code, const char *parameter)
{
	int i;
	for (i = 0; i < (sizeof(patcharr) / sizeof(struct cfg_patch)); i++) {
		int mask = (patcharr[i].mask != 0) ? patcharr[i].mask : ~0;

		if ((code & mask) == patcharr[i].code) {
			int retval = patcharr[i].patch(config, &patcharr[i], code, parameter);
			if (retval != 0)
				printf("failed to patch code=0x%x parameter='%s'\n", code, parameter);

			return retval;
		}
	}
	printf("nothing found to patch code=0x%x parameter='%s'\n", code, parameter);
	return -1;
}
