/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com> (hashtable)
 *                        Echipa SDA ll_list
 */

#include "structuri.h"


/*
 * Functii de comparare a cheilor:
 */
int compare_function_ints(void *a, void *b) {
	int int_a = *((int *)a);
	int int_b = *((int *)b);

	if (int_a == int_b) {
		return 0;
	} else if (int_a < int_b) {
		return -1;
	} else {
		return 1;
	}
}

int compare_function_strings(void *a, void *b) {
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/*
 * Functii de hashing:
 */
unsigned int hash_function_int(void *a) {
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_string(void *a) {
	unsigned char *puchar_a = (unsigned char*) a;
	unsigned long hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

	return hash;
}

/*
 * Functie apelata pentru a elibera memoria ocupata de cheia si valoarea unei
 * perechi din hashtable. Daca cheia sau valoarea contin tipuri de date complexe
 * aveti grija sa eliberati memoria luand in considerare acest aspect.
 */
void key_val_free_function(void *data) {
	free(((info *)data)->key);
	free(((info *)data)->value);
	/// REVINO AICI
}

/*
 * Functie apelata dupa alocarea unui hashtable pentru a-l initializa.
 * Trebuie alocate si initializate si listele inlantuite.
 */
hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*),
		void (*key_val_free_function)(void*)) {
	hashtable_t *hash_table = malloc(sizeof(hashtable_t));
    DIE(!hash_table, "Eroare la alocare\n");

	hash_table->buckets = malloc(hmax * sizeof(linked_list_t *));
    DIE(!hash_table->buckets, "Eroare la alocare\n");

	for (unsigned int i = 0; i < hmax; i++)
		hash_table->buckets[i] = ll_create(sizeof(info));
	hash_table->hmax = hmax;
	hash_table->hash_function = hash_function;
	hash_table->key_val_free_function = key_val_free_function;
	hash_table->compare_function = compare_function;
	hash_table->size = 0;

	return hash_table;
}

/*
 * Functie care intoarce:
 * 1, daca pentru cheia key a fost asociata anterior o valoare in hashtable
 * folosind functia put;
 * 0, altfel.
 */
int ht_has_key(hashtable_t *ht, void *key) {
	unsigned int index = ht->hash_function(key) % ht->hmax;
	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current_node = bucket->head;
	while (current_node) {
		void *current_key = ((info *)current_node->data)->key;
		if (!ht->compare_function(current_key, key))
			return 1;
		current_node = current_node->next;
	}
	return 0;
}

void *ht_get(hashtable_t *ht, void *key) {
	unsigned int index = ht->hash_function(key) % ht->hmax;
	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current_node = bucket->head;
	while (current_node) {
		void *current_key = ((info *)current_node->data)->key;
		if (!ht->compare_function(current_key, key)) {
			void *value = ((info *)current_node->data)->value;
			return value;
		}
		current_node = current_node->next;
	}

	return NULL;
}

/*
 * Atentie! Desi cheia este trimisa ca un void pointer (deoarece nu se impune
 * tipul ei), in momentul in care se creeaza o noua intrare in hashtable (in
 * cazul in care cheia nu se gaseste deja in ht), trebuie creata o copie a
 * valorii la care pointeaza key si adresa acestei copii trebuie salvata in
 * structura info asociata intrarii din ht. Pentru a sti cati octeti trebuie
 * alocati si copiati, folositi parametrul key_size.
 *
 * Motivatie:
 * Este nevoie sa copiem valoarea la care pointeaza key deoarece dupa un apel
 * put(ht, key_actual, value_actual), valoarea la care pointeaza key_actual
 * poate fi alterata (de ex: *key_actual++). Daca am folosi direct adresa
 * pointerului key_actual, practic s-ar modifica din afara hashtable-ului cheia
 * unei intrari din hashtable. Nu ne dorim acest lucru, fiindca exista riscul sa
 * ajungem in situatia in care nu mai stim la ce cheie este inregistrata o
 * anumita valoare.
 */
void ht_put(hashtable_t *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size) {
	unsigned int index = ht->hash_function(key) % ht->hmax;
	linked_list_t *bucket = ht->buckets[index];

	ll_node_t *current_node = bucket->head;

	while (current_node != NULL) {
		void *current_key = ((info*)current_node->data)->key;
		if (ht->compare_function(key, current_key) == 0) {
			void *current_value = ((info*)current_node->data)->value;
			free(current_value);
			((info*)current_node->data)->value = malloc(value_size);
            DIE(!((info*)current_node->data)->value, "Eroare la alocare\n");

			memcpy(((info*)current_node->data)->value, value, value_size);
			return;
		}

		current_node = current_node->next;
	}

	info new_info;
	new_info.key = malloc(key_size);
    DIE(!new_info.key, "Eroare la alocare\n");

	memcpy(new_info.key, key, key_size);
	new_info.value = malloc(value_size);
    DIE(!new_info.value, "Eroare la alocare\n");

	memcpy(new_info.value, value, value_size);
	ll_add_nth_node(bucket, 0, &new_info);
	ht->size++;
}

/*
 * Procedura care elimina din hashtable intrarea asociata cheii key.
 * Atentie! Trebuie avuta grija la eliberarea intregii memorii folosite pentru o
 * intrare din hashtable (adica memoria pentru copia lui key --vezi observatia
 * de la procedura put--, pentru structura info si pentru structura Node din
 * lista inlantuita).
 */
void ht_remove_entry(hashtable_t *ht, void *key) {
	if (!ht)
		return;

	unsigned int index = ht->hash_function(key) % ht->hmax;
	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current_node = bucket->head;
	unsigned int cnt = 0;
	while (current_node) {
		void *current_key = ((info *)current_node->data)->key;
		if (!ht->compare_function(current_key, key)) {
			current_node = ll_remove_nth_node(bucket, cnt);
			ht->key_val_free_function(current_node->data);
			free(current_node->data);
			free(current_node);
			ht->size--;
			return;
		}
		current_node = current_node->next;
		cnt++;
	}
}

/*
 * Procedura care elibereaza memoria folosita de toate intrarile din hashtable,
 * dupa care elibereaza si memoria folosita pentru a stoca structura hashtable.
 */
void ht_free(hashtable_t *ht) {
	for (unsigned int i = 0; i < ht->hmax; i++) {
		linked_list_t *bucket = ht->buckets[i];
		ll_node_t *current_node;
		while ((current_node = ll_remove_nth_node(bucket, 0))) {
			ht->key_val_free_function(current_node->data);
			free(current_node->data);
			free(current_node);
		}
		free(bucket);
	}
	free(ht->buckets);
	free(ht);
}

unsigned int ht_get_size(hashtable_t *ht) {
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int ht_get_hmax(hashtable_t *ht) {
	if (ht == NULL)
		return 0;

	return ht->hmax;
}
