#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "configdata.h"
#include "filedata.h"

/*
 * $ cfgpatch <config-in> <config-out>
 */
int main(int argc, char *argv[])
{
	struct filedata *config = get_filedata(argv[1]);

	config_patch(config->data, CFG_LOCATION, "location");
	config_patch(config->data, CFG_SYSNAME, "sysname");
	config_patch(config->data, CFG_CONTACT, "contact");

	config_patch(config->data, CFG_IPADDRESS, "10.10.200.10");
	config_patch(config->data, CFG_NETMASK, "16");
	config_patch(config->data, CFG_DEFAULTGW, "10.10.250.250");
	config_patch(config->data, CFG_NAMESERVER, "10.10.0.1");

	config_patch(config->data, CFG_MACAGEING, "301");

	config_patch(config->data, CFG_PORTNAME_MASK + 1, "PORT-001");
	config_patch(config->data, CFG_PORTNAME_MASK + 2, "PORT-002");

	put_filedata(argv[2], config);

	free(config);
	return 0;
}
