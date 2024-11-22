/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#ifndef STRUCTURI_H
#define STRUCTURI_H

/// Implementari pentru hashtable, lista dublu inlantuita si coada.
#include "../utils/utils.h"
#include "../utils/constants.h"


///
///
/// Simple linked list
///
///
typedef struct ll_node_t
{
    void* data;
    struct ll_node_t* next;
} ll_node_t;

typedef struct linked_list_t
{
    ll_node_t* head;
    unsigned int data_size;
    unsigned int size;
} linked_list_t;

linked_list_t *ll_create(unsigned int data_size);
void ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data);
ll_node_t *ll_remove_nth_node(linked_list_t* list, unsigned int n);
unsigned int ll_get_size(linked_list_t* list);
void ll_free(linked_list_t** pp_list);

///
///
/// Hashtable
///
///
typedef struct info info;
struct info {
	void *key;
	void *value;
};

typedef struct hashtable_t hashtable_t;
struct hashtable_t {
	linked_list_t **buckets; /* Array de liste simplu-inlantuite. */
	/* Nr. total de noduri existente curent in toate bucket-urile. */
	unsigned int size;
	unsigned int hmax; /* Nr. de bucket-uri. */
	/* (Pointer la) Functie pentru a calcula valoarea hash asociata cheilor. */
	unsigned int (*hash_function)(void*);
	/* (Pointer la) Functie pentru a compara doua chei. */
	int (*compare_function)(void*, void*);
	/* (Pointer la) Functie pentru a elibera memoria
     * ocupata de cheie si valoare. */
	void (*key_val_free_function)(void*);
};

int compare_function_ints(void *a, void *b);
int compare_function_strings(void *a, void *b);
unsigned int hash_function_int(void *a);
unsigned int hash_function_string(void *a);
void key_val_free_function(void *data);
hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
					   int (*compare_function)(void*, void*),
					   void (*key_val_free_function)(void*));
int ht_has_key(hashtable_t *ht, void *key);
void *ht_get(hashtable_t *ht, void *key);
void ht_put(hashtable_t *ht, void *key, unsigned int key_size,
			void *value, unsigned int value_size);
void ht_remove_entry(hashtable_t *ht, void *key);
void ht_free(hashtable_t *ht);
unsigned int ht_get_size(hashtable_t *ht);
unsigned int ht_get_hmax(hashtable_t *ht);

///
///
/// Double linked list
///
///
typedef struct dll_node dll_node;
struct dll_node {
	void *data;
	dll_node *prev, *next;
};

typedef struct dll_list dll_list;
struct dll_list {
	dll_node *head;
	int data_size;
	int size;
};

void dll_free(dll_list **pp_list, void (*elem_free)(void *));
void dll_print_string_list(dll_list* list);
void dll_print_int_list(dll_list* list);
void dll_add_nth_node(dll_list *list, int n, void *new_data);
void remove_a_node(dll_node *node_to_remove, dll_list *list);
dll_node *dll_remove_nth_node_from_end(dll_list *list);
dll_node *dll_remove_nth_node(dll_list *list, int n);
dll_list *dll_create(int data_size);

///
///
/// queue
///
///

typedef struct queue_t queue_t;
struct queue_t
{
	/* Dimensiunea maxima a cozii */
	unsigned int max_size;
	/* Dimensiunea cozii */
	unsigned int size;
	/* Dimensiunea in octeti a tipului de date stocat in coada */
	unsigned int data_size;
	/* Indexul de la care se vor efectua operatiile de front si dequeue */
	unsigned int read_idx;
	/* Indexul de la care se vor efectua operatiile de enqueue */
	unsigned int write_idx;
	/* Bufferul ce stocheaza elementele cozii */
	void **buff;
};

queue_t *q_create(unsigned int data_size, unsigned int max_size);
unsigned int q_get_size(queue_t *q);
unsigned int q_is_empty(queue_t *q);
void *q_front(queue_t *q);
int q_dequeue(queue_t *q);
int q_enqueue(queue_t *q, void *new_data);
void q_clear(queue_t *q);
void q_free(queue_t *q);

#endif
