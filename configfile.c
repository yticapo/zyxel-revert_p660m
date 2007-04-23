/***************************************************************************
 *   Copyright (C) 06/2006 by Olaf Rempel                                  *
 *   razzor@kopf-tisch.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "configfile.h"
#include "list.h"
#include "logging.h"

#define BUFSIZE 1024

struct conf_section {
	struct list_head list;
	struct list_head tupel_list;
	const char *name;
};

struct conf_tupel {
	struct list_head list;
	const char *option;
	const char *parameter;
};

static LIST_HEAD(config_list);

static struct conf_section * config_add_section(const char *name)
{
	struct conf_section *section;
	section = malloc(sizeof(struct conf_section) + strlen(name));
	if (section == NULL)
		return NULL;

	INIT_LIST_HEAD(&section->list);
	INIT_LIST_HEAD(&section->tupel_list);

	section->name = strdup(name);
	if (section->name == NULL) {
		free(section);
		return NULL;
	}

	list_add_tail(&section->list, &config_list);
	return section;
}

static int config_add_tupel(struct conf_section *section, const char *option, const char *parameter)
{
	struct conf_tupel *tupel = malloc(sizeof(struct conf_tupel));
	if (tupel == NULL)
		return -1;

	INIT_LIST_HEAD(&tupel->list);
	tupel->option = strdup(option);
	tupel->parameter = strdup(parameter);

	if (tupel->option == NULL || tupel->parameter == NULL) {
		free(tupel);
		return -1;
	}

	list_add_tail(&tupel->list, &section->tupel_list);
	return 0;
}

int config_parse(const char *config)
{
	FILE *fz = fopen(config, "r");
	if (fz == NULL) {
		log_print(LOG_ERROR, "config_parse(): %s", config);
		return -1;
	}

	char *line = malloc(BUFSIZE);
	if (line == NULL) {
		log_print(LOG_ERROR, "config_parse(): out of memory");
		fclose(fz);
		return -1;
	}

	int linenum = 0;
	struct conf_section *section = NULL;
	while (fgets(line, BUFSIZE, fz) != NULL) {
		linenum++;

		if (line[0] == '#' || line[0] <= ' ') {
			continue;

		} else if (line[0] == '[') {
			char *tok = strtok(line +1, " ]\n");

			if (tok == NULL || (section = config_add_section(tok)) == NULL) {
				log_print(LOG_WARN, "config_parse(): invalid section in row %d", linenum);
				free(line);
				fclose(fz);
				return -1;
			}
			continue;

		} else if (section == NULL) {
			log_print(LOG_WARN, "config_parse(): missing section in row %d", linenum);
			free(line);
			fclose(fz);
			return -1;
		}

		char *tok = strtok(line, " \n");
		if (tok != NULL) {
			char *tok2;
			while ((tok2 = strtok(NULL, " \n"))) {
				if (config_add_tupel(section, tok, tok2) != 0)
					log_print(LOG_WARN, "config_parse(): invalid row %d", linenum);
			}
		}
	}

	fclose(fz);
	free(line);
	return 0;
}

void config_free(void)
{
	struct conf_section *section, *section_tmp;
	struct conf_tupel *tupel, *tupel_tmp;

	list_for_each_entry_safe(section, section_tmp, &config_list, list) {
		list_for_each_entry_safe(tupel, tupel_tmp, &section->tupel_list, list) {
			list_del(&tupel->list);
			free((char *)tupel->option);
			free((char *)tupel->parameter);
			free(tupel);
		}
		list_del(&section->list);
		free(section);
	}
}

static struct conf_section * config_get_section(const char *name)
{
	struct conf_section *section;

	list_for_each_entry(section, &config_list, list) {
		if (!strcmp(section->name, name))
			return section;
	}
	return NULL;
}

const char * config_get_string(const char *section_str, const char *option, const char *def)
{
	struct conf_section *section = config_get_section(section_str);
	if (section != NULL) {
		struct conf_tupel *tupel;
		list_for_each_entry(tupel, &section->tupel_list, list) {
			if (!strcmp(tupel->option, option))
				return tupel->parameter;
		}
	}

	if (def != NULL)
		log_print(LOG_WARN, "config [%s:%s] not found, using default: '%s'",
			section_str, option, def);
	return def;
}

int config_get_int(const char *section, const char *option, int def)
{
	const char *ret = config_get_string(section, option, NULL);
	if (ret == NULL) {
		log_print(LOG_WARN, "config [%s:%s] not found, using default: '%d'",
			section, option, def);
		return def;
	}
	return atoi(ret);
}

int config_get_strings(const char *section_str, const char *option,
			int (*callback)(const char *value, void *privdata),
			void *privdata)
{
	struct conf_section *section = config_get_section(section_str);
	if (section == NULL)
		return -1;

	int cnt = 0;
	struct conf_tupel *tupel;
	list_for_each_entry(tupel, &section->tupel_list, list) {
		if (!strcmp(tupel->option, option))
			if (callback(tupel->parameter, privdata) == 0)
				cnt++;
	}
	return cnt;
}
