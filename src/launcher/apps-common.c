/**************************************************************************
* Tint2 : .desktop file handling
*
* Copyright (C) 2015       (mrovi9000@gmail.com)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

/* http://standards.freedesktop.org/desktop-entry-spec/ */

#include "apps-common.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_dektop_line(char *line, char **key, char **value)
{
	char *p;
	int found = 0;
	*key = line;
	for (p = line; *p; p++) {
		if (*p == '=') {
			*value = p + 1;
			*p = 0;
			found = 1;
			break;
		}
	}
	if (!found)
		return 0;
	if (found && (strlen(*key) == 0 || strlen(*value) == 0))
		return 0;
	return 1;
}

void expand_exec(DesktopEntry *entry, const char *path)
{
	// Expand % in exec
	// %i -> --icon Icon
	// %c -> Name
	// %k -> path
	if (entry->exec) {
		char *exec2 = malloc(strlen(entry->exec) + (entry->name ? strlen(entry->name) : 1) + (entry->icon ? strlen(entry->icon) : 1) + 100);
		char *p, *q;
		// p will never point to an escaped char
		for (p = entry->exec, q = exec2; *p; p++, q++) {
			*q = *p; // Copy
			if (*p == '\\') {
				p++, q++;
				// Copy the escaped char
				if (*p == '%') // For % we delete the backslash, i.e. write % over it
					q--;
				*q = *p;
				if (!*p) break;
				continue;
			}
			if (*p == '%') {
				p++;
				if (!*p) break;
				if (*p == 'i' && entry->icon != NULL) {
					sprintf(q, "--icon '%s'", entry->icon);
					q += strlen("--icon ''");
					q += strlen(entry->icon);
					q--; // To balance the q++ in the for
				} else if (*p == 'c' && entry->name != NULL) {
					sprintf(q, "'%s'", entry->name);
					q += strlen("''");
					q += strlen(entry->name);
					q--; // To balance the q++ in the for
				} else if (*p == 'c') {
					sprintf(q, "'%s'", path);
					q += strlen("''");
					q += strlen(path);
					q--; // To balance the q++ in the for
				} else {
					// We don't care about other expansions
					q--; // Delete the last % from q
				}
				continue;
			}
		}
		*q = '\0';
		free(entry->exec);
		entry->exec = exec2;
	}
}

int read_desktop_file(const char *path, DesktopEntry *entry)
{
	FILE *fp;
	char *line = NULL;
	size_t line_size;
	char *key, *value;
	int i;

	entry->name = entry->icon = entry->exec = NULL;

	if ((fp = fopen(path, "rt")) == NULL) {
		fprintf(stderr, "Could not open file %s\n", path);
		return 0;
	}

	gchar **languages = (gchar **)g_get_language_names();
	// lang_index is the index of the language for the best Name key in the language vector
	// lang_index_default is a constant that encodes the Name key without a language
	int lang_index, lang_index_default;
#define LANG_DBG 0
	if (LANG_DBG) printf("Languages:");
	for (i = 0; languages[i]; i++) {
		if (LANG_DBG) printf(" %s", languages[i]);
	}
	if (LANG_DBG) printf("\n");
	lang_index_default = i;
	// we currently do not know about any Name key at all, so use an invalid index
	lang_index = lang_index_default + 1;

	int inside_desktop_entry = 0;
	while (getline(&line, &line_size, fp) >= 0) {
		int len = strlen(line);
		if (len == 0)
			continue;
		line[len - 1] = '\0';
		if (line[0] == '[') {
			inside_desktop_entry = (strcmp(line, "[Desktop Entry]") == 0);
		}
		if (inside_desktop_entry && parse_dektop_line(line, &key, &value)) {
			if (strstr(key, "Name") == key) {
				if (strcmp(key, "Name") == 0 && lang_index > lang_index_default) {
					entry->name = strdup(value);
					lang_index = lang_index_default;
				} else {
					for (i = 0; languages[i] && i < lang_index; i++) {
						gchar *localized_key = g_strdup_printf("Name[%s]", languages[i]);
						if (strcmp(key, localized_key) == 0) {
							if (entry->name)
								free(entry->name);
							entry->name = strdup(value);
							lang_index = i;
						}
						g_free(localized_key);
					}
				}
			} else if (!entry->exec && strcmp(key, "Exec") == 0) {
				entry->exec = strdup(value);
			} else if (!entry->icon && strcmp(key, "Icon") == 0) {
				entry->icon = strdup(value);
			}
		}
	}
	fclose (fp);
	// From this point:
	// entry->name, entry->icon, entry->exec will never be empty strings (can be NULL though)

	expand_exec(entry, path);

	free(line);
	return 1;
}

void free_desktop_entry(DesktopEntry *entry)
{
	free(entry->name);
	free(entry->icon);
	free(entry->exec);
	entry->name = entry->icon = entry->exec = NULL;
}

void test_read_desktop_file()
{
	fprintf(stdout, "\033[1;33m");
	DesktopEntry entry;
	read_desktop_file("/usr/share/applications/firefox.desktop", &entry);
	printf("Name:%s Icon:%s Exec:%s\n", entry.name, entry.icon, entry.exec);
	fprintf(stdout, "\033[0m");
}
