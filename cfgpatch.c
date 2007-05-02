#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "configdata.h"
#include "filedata.h"

/*
 * $ cfgpatch <config-in>
 */
int main(int argc, char *argv[])
{
	struct filedata *config = get_filedata(argv[1]);

	config_patch(config->data, CFG_PASSWORD, "1234");

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
	config_patch(config->data, CFG_PORTNAME_MASK + 26, "UPLINK");

	config_patch(config->data, CFG_PORTENABLE_MASK + 25, "off");

	config_patch(config->data, CFG_SNMP_READ_COMMUNITY, "public1234");
	config_patch(config->data, CFG_SNMP_WRITE_COMMUNITY, "private1234");

	config_patch(config->data, CFG_CPUVLANID, "124");

	config_patch(config->data, CFG_PORTVLAN_MASK + 1, "124:check:all");
	config_patch(config->data, CFG_PORTVLAN_MASK + 2, "124:nocheck:tagged");

	config_patch(config->data, CFG_DOT1QVLAN_MASK + 2, "124:test");
	config_patch(config->data, CFG_DOT1QPORT_MASK + 2, "1:fixed:tagging");
	config_patch(config->data, CFG_DOT1QPORT_MASK + 2, "2:normal:notagging");
	config_patch(config->data, CFG_DOT1QPORT_MASK + 2, "3:forbidden:notagging");

	char outname[64];
	strncpy(outname, argv[1], sizeof(outname));
	strcat(outname, ".patched");
	put_filedata(outname, config);

	free(config);
	return 0;
}
