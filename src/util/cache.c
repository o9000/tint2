/**************************************************************************
*
* Tint2 : cache
*
* Copyright (C) 2016 @o9000
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

#include "cache.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

void init_cache(Cache *cache)
{
	if (cache->_table)
		free_cache(cache);
	cache->_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	cache->dirty = FALSE;
	cache->loaded = FALSE;
}

void free_cache(Cache *cache)
{
	if (cache->_table)
		g_hash_table_destroy(cache->_table);
	cache->_table = NULL;
	cache->dirty = FALSE;
	cache->loaded = FALSE;
}

void load_cache(Cache *cache, const gchar *cache_path)
{
	init_cache(cache);

	cache->loaded = TRUE;

	int fd = open(cache_path, O_RDONLY);
	if (fd == -1)
		return;
	flock(fd, LOCK_SH);

	FILE *f = fopen(cache_path, "rt");
	if (!f)
		goto unlock;

	char *line = NULL;
	size_t line_size;

	while (getline(&line, &line_size, f) >= 0) {
		char *key, *value;

		size_t line_len = strlen(line);
		gboolean has_newline = FALSE;
		if (line_len >= 1) {
			if (line[line_len - 1] == '\n') {
				line[line_len - 1] = '\0';
				line_len--;
				has_newline = TRUE;
			}
		}
		if (!has_newline)
			break;

		if (line_len == 0)
			continue;

		if (parse_line(line, &key, &value)) {
			g_hash_table_insert(cache->_table, g_strdup(key), g_strdup(value));
			free(key);
			free(value);
		}
	}
	free(line);
	fclose(f);

unlock:
	flock(fd, LOCK_UN);
	close(fd);
}

void write_cache_line(gpointer key, gpointer value, gpointer user_data)
{
	gchar *k = key;
	gchar *v = value;
	FILE *f = user_data;

	fprintf(f, "%s=%s\n", k, v);
}

void save_cache(Cache *cache, const gchar *cache_path)
{
	int fd = open(cache_path, O_RDONLY | O_CREAT, 0600);
	if (fd == -1) {
		gchar *dir_path = g_path_get_dirname(cache_path);
		g_mkdir_with_parents(dir_path, 0700);
		g_free(dir_path);
		fd = open(cache_path, O_RDONLY | O_CREAT, 0600);
	}
	if (fd == -1) {
		fprintf(stderr, RED "Could not save icon theme cache!" RESET "\n");
		return;
	}
	flock(fd, LOCK_EX);

	FILE *f = fopen(cache_path, "w");
	if (!f) {
		fprintf(stderr, RED "Could not save icon theme cache!" RESET "\n");
		goto unlock;
	}
	g_hash_table_foreach(cache->_table, write_cache_line, f);
	fclose(f);
	cache->dirty = FALSE;

unlock:
	flock(fd, LOCK_UN);
	close(fd);
}

const gchar *get_from_cache(Cache *cache, const gchar *key)
{
	if (!cache->_table)
		return NULL;
	return g_hash_table_lookup(cache->_table, key);
}

void add_to_cache(Cache *cache, const gchar *key, const gchar *value)
{
	if (!cache->_table)
		init_cache(cache);

	if (!key || !value)
		return;

	gchar *old_value = g_hash_table_lookup(cache->_table, key);
	if (old_value && g_str_equal(old_value, value))
		return;

	g_hash_table_insert(cache->_table, g_strdup(key), g_strdup(value));
	cache->dirty = TRUE;
}
