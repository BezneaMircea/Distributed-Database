/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdbool.h>
#include "../structures/structuri.h"

/** 
 *  @param max_capacity: Capacitatea maxima a cacheului
 *  @param size: Dimensiunea cacheului
 *  @param order_list: Dll_list circulara pentru ordine
 *  @param cache_ht: Hashtable in care pastram contentul unui document
 *					 si pointer catre nodul corespunzator din lista de ordine
 */
typedef struct lru_cache {
	int max_capacity;
	int size;
	dll_list *order_list;
	hashtable_t *cache_ht;
} lru_cache;

/** 
 *  @param doc_conter: Contentul documentului
 *  @param node_order: Pointer spre nodul din lista de ordine
 */
typedef struct cache_info {
	char doc_conter[DOC_CONTENT_LENGTH];
	dll_node *node_order;
} cache_info;

lru_cache *init_lru_cache(unsigned int cache_capacity);

bool lru_cache_is_full(lru_cache *cache);

void free_lru_cache(lru_cache **cache);

bool lru_cache_put(lru_cache *cache, void *key, void *value,
				   void **evicted_key);

void *lru_cache_get(lru_cache *cache, void *key);

void lru_cache_remove(lru_cache *cache, void *key);

#endif /* LRU_CACHE_H */
