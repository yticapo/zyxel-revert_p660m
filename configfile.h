#ifndef _CONFIG_H_
#define _CONFIG_H_

int config_parse(const char *config);
void config_free(void);

const char * config_get_string(const char *section_str, const char *option, const char *def);

int config_get_int(const char *section, const char *option, int def);

int config_get_strings(const char *section_str, const char *option,
			int (*callback)(const char *value, void *privdata),
			void *privdata);

#endif /* _CONFIG_H_ */
