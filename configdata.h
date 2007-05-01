#ifndef _CONFIGDATA_H_
#define _CONFIGDATA_H_

enum {
	CFG_PASSWORD = 0x0001,
	CFG_LOCATION,
	CFG_SYSNAME,
	CFG_CONTACT,

	CFG_DEFAULTGW,
	CFG_IPADDRESS,
	CFG_NETMASK,
	CFG_NAMESERVER,

	CFG_MACAGEING,

	CFG_CPUVLAN,

	CFG_SNMP_READ_COMMUNITY,
	CFG_SNMP_WRITE_COMMUNITY,

	CFG_PORTENABLE_MASK = 0x0100,
	CFG_PORTNAME_MASK = 0x0120,
	CFG_PORTVLAN_MASK = 0x0140,
	CFG_DOT1QVLAN_MASK = 0x0160,
	CFG_DOT1QPORT_MASK = 0x0180,
};

int config_patch(void *config, int code, const char *parameter);

#endif /* _CONFIGDATA_H_ */
