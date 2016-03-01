#ifndef CACHE_H
#define CACHE_H

#include <glib.h>

// A cache with string keys and values, backed by a file.
// The strings must not be NULL and are stripped of any whitespace at start and end.
typedef struct Cache {
	gboolean dirty;
	gboolean loaded;
	GHashTable *_table;
} Cache;

// Initializes the cache. You can also call load_cache directly if you set the memory contents to zero first.
void init_cache(Cache *cache);

// Clears the cache contents and releases all memory, but not the object.
// You can use init_cache or load_cache afterwards.
void free_cache(Cache *cache);

// Clears the cache contents and loads new contents from a file.
// Sets the loaded flag to TRUE.
void load_cache(Cache *cache, const gchar *cache_path);

// Saves the cache contents to a file.
// Clears the dirty flag.
void save_cache(Cache *cache, const gchar *cache_path);

// Returns a pointer to the value in the cache, or NULL if not found.
// Do not free the returned value!
const gchar *get_from_cache(Cache *cache, const gchar *key);

// Adds a key-value pair to the cache. NULL keys or values are not allowed.
// If the key already exists, the old value is and replaced with the new value.
// Does not take ownership of the pointers (neither key, nor value); instead it makes copies.
// Sets the dirty flag to TRUE.
void add_to_cache(Cache *cache, const gchar *key, const gchar *value);

#endif
