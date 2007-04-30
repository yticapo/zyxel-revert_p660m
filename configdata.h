#ifndef _CONFIGDATA_H_
#define _CONFIGDATA_H_

enum {
	CFG_LOCATION = 0x0001,
	CFG_SYSNAME,
	CFG_CONTACT,

	CFG_DEFAULTGW,
	CFG_IPADDRESS,
	CFG_NETMASK,
	CFG_NAMESERVER,

	CFG_MACAGEING,

	CFG_PORTNAME_MASK = 0x0100,	// next 0x0120
};

int config_patch(void *config, int code, const char *parameter);

#endif /* _CONFIGDATA_H_ */
