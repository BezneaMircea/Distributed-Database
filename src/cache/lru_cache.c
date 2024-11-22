/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#include <stdio.h>
#include <string.h>
#include "lru_cache.h"
#include "../utils/utils.h"

/** @param cache_capacity: capacitatea maxima a cache-ului
 *  @brief Functia care initializeaza cacheul
 *  @return Returneaza un cache
 * */
lru_cache *init_lru_cache(unsigned int cache_capacity) {
	lru_cache *cache = malloc(sizeof(lru_cache));
    DIE(!cache, "Eroare la alocare\n");

	cache->max_capacity = cache_capacity;
	cache->size = 0;
	cache->cache_ht = ht_create(cache_capacity, hash_function_string,
								compare_function_strings,
								key_val_free_function);
	cache->order_list = dll_create(DOC_NAME_LENGTH);

	return cache;
}

/** @param cache: cache-ul
 *  @brief Verifica daca cacheul e plin
 *  @return Returneaza true daca cacheul e plin false in caz contrar
 * */
bool lru_cache_is_full(lru_cache *cache) {
	if (cache->size == cache->max_capacity)
		return true;
	return false;
}

/** @param cache: cache-ul
 *  @brief Elibereaza memoria ocupata de cache 
 *		   si de continutul acestuia
 * */
void free_lru_cache(lru_cache **cache) {
	lru_cache *cache_mem = *cache;
	dll_free(&cache_mem->order_list, NULL);
	ht_free(cache_mem->cache_ht);

	free(cache_mem);
	*cache = NULL;
}

/** @param cache: cache-ul
 *  @param key: cheia
 * 	@param value: valoarea
 *  @param evicted_key: Param prin care se intoarce cheie eliminata
 *  @brief Adauga o noua pereche in cache.
 *		   Intoarce true daca cheia a fost adaugata,
 *		   false daca cheia exista deja.
 *		   Evicted_key este parametrul prin care pastram
 *		   cheia scoasa din cache in caz ca acesta era plin
 * */
bool lru_cache_put(lru_cache *cache, void *key, void *value,
				   void **evicted_key) {
	cache_info new_info;
	memcpy(new_info.doc_conter, value, DOC_CONTENT_LENGTH);
	dll_add_nth_node(cache->order_list, 0, key);
	new_info.node_order = cache->order_list->head;
	/* Cream noua informatie de adaugat in hash_map */

	bool ret_value = true;
	if (ht_has_key(cache->cache_ht, key)) {
		/* Daca cheia exista deja */
		cache_info *old_info = (cache_info *)ht_get(cache->cache_ht, key);
		remove_a_node(old_info->node_order, cache->order_list);
		/* Scoatem din lista de ordine nodul asociat intrarii actuale */
		cache->size--;

		ret_value = false;
	} else if (lru_cache_is_full(cache)) {
		/* Daca cheia nu exista dar cacheul e plin */
		dll_node *node_to_remove = dll_remove_nth_node_from_end(cache->order_list);
		/* Scoatem ultimul nod din lista (cel mai vechi accesat) */

		/* Eliberam memoria pentru ultimul nod */
		ht_remove_entry(cache->cache_ht, node_to_remove->data);
		/* Scoatem intrarea asociata acestui nod si din hash_map */
		cache->size--;

		memcpy(*evicted_key, node_to_remove->data, DOC_NAME_LENGTH);
		free(node_to_remove->data);
		free(node_to_remove);
		/* Eliberam memoria folosita de nod dupa ce copiam continutul 
		 * in evicted key */
	} else {
		/* Eliberam memoria alocata pentru string
		 * Daca doar adaugam cheia si facem ca ac sa pointeze
		 * spre NULL */
		free(*evicted_key);
		*evicted_key = NULL;
	}

	cache->size++;
	ht_put(cache->cache_ht, key, DOC_NAME_LENGTH, &new_info, sizeof(cache_info));
	/* Adaugam noile data in hash_map */

	return ret_value;
}

/** 
 *  @brief Functia intoarce informatia asociata cheii key
 *  	   si face modificarile necesare in lista de ordine
 *  	   pentru ca documentul asociat cheii key sa fie
 *  	   primul, intoarce NULL daca nu exista intrarea pentru
 *  	   key 
 * @param cache: cache-ul
 * @param key: cheia continutului dorit
 * @return Continutul de la cheia key
 * */
void *lru_cache_get(lru_cache *cache, void *key) {
	cache_info *old_info = (cache_info *)ht_get(cache->cache_ht, key);
	/* Extragem din hash_map datele de la cheia key */
	if (!old_info)
		return NULL;
	/* Daca nu exista cheia in hash_map */

	cache_info new_info;
	memcpy(new_info.doc_conter, old_info->doc_conter, DOC_CONTENT_LENGTH);
	dll_add_nth_node(cache->order_list, 0, key);
	/* Adaugarea pe pozitia 0 se face in O(1) */
	new_info.node_order = cache->order_list->head;
	/* Am creat noua informatie ce trebuie adaugata in hash_table */

	remove_a_node(old_info->node_order, cache->order_list);
	/* Scoatem din lista de ordine nodul spre care pointeaza informatia
	 * curenta din hash_map */

	ht_put(cache->cache_ht, key, DOC_NAME_LENGTH, &new_info, sizeof(cache_info));
	/* Adaugam in hash_map noua informatie */

	return ((cache_info *)ht_get(cache->cache_ht, key))->doc_conter;
	/* Intoarcem continutul documentului */
}


/** @param cache: Memoria cache
 *  @param key: Cheia key
 *  @brief Elibereaza din cache intrarea asociata cheii key
 * */
void lru_cache_remove(lru_cache *cache, void *key) {
	cache_info *old_info = (cache_info *)ht_get(cache->cache_ht, key);
	/* Extragem din hash_map datele de la cheia key */
	if (!old_info)
		return;
	/* Daca nu exista cheia in hash_map */

	remove_a_node(old_info->node_order, cache->order_list);
	/* Scoatem nodul spre care pointeaza informatia de la cheia key
	 * din lista de ordine */
	cache->size--;

	ht_remove_entry(cache->cache_ht, key);
	/* Scoatem informatia si din hash_map */
}
