/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#include "load_balancer.h"
#include "../server/server.h"

/** @param enable_vnoed: Nu am facut bonusul... so we don t care
 *  @brief Functia initializeaza load_balancerul
 *  @return Returneaza load_balancerul
 */
load_balancer *init_load_balancer(bool enable_vnodes) {
	(void)enable_vnodes;

	load_balancer *load_bal = malloc(sizeof(load_balancer));
    DIE(!load_bal, "Eroare la alocare\n");

	load_bal->hash_function_docs = hash_function_string;
	load_bal->hash_function_servers = hash_function_int;
	load_bal->nr_servers = 0;
	load_bal->servers = malloc(MAX_SERVERS * sizeof(server *));
    DIE(!load_bal->servers, "Eroare la alocare\n");

	load_bal->order = malloc(MAX_SERVERS * sizeof(server_order));
    DIE(!load_bal->order, "Eroare la alocare\n");

	return load_bal;
}

/** @param a: Primul element de schimbat
 *  @param b: Al doilea element de schimbat
 *  @brief Interschimba doua elemente din vectorul de ordine
 */
void swap_servers_order(server_order *a, server_order *b) {
	int aux = a->id_hash;
	a->id_hash = b->id_hash;
	b->id_hash = aux;

	aux = a->id_server;
	a->id_server = b->id_server;
	b->id_server = aux;
}

/** @param order: Vectorul de ordine (Hashring-ul)
 *  @param nr_servers: Nr de servere
 *  @brief Functia pune ultimul element din vectorul de ordine
 *		   pe pozitia sa corespunzatoare
 *  @return Intoarce id_ul serverului destinatie (Cel care va prelua din documente)
 */
int put_last_in_place(server_order *order, int nr_servers) {
	if (order[nr_servers - 1].id_hash > order[nr_servers - 2].id_hash)
		return order[0].id_server;
	/* Caz de la capat */

	for (int i = nr_servers - 1; i > 0; i--) {
		if (order[i].id_hash < order[i - 1].id_hash) {
			swap_servers_order(&order[i], &order[i - 1]);
		} else if (order[i].id_hash == order[i - 1].id_hash) {
			if (order[i].id_server < order[i - 1].id_server) {
				swap_servers_order(&order[i], &order[i - 1]);
			} else {
				return order[i + 1].id_server;
			}
		} else {
			return order[i + 1].id_server;
		}
	}

	return order[1].id_server;
	/* Caz de la capat */
}

/** @param main: Load_balancerul
 *  @param server_id: Id-ul serverului destinatie
 *  @param hash: Hashul unui document de pe serverul sursa
 *  @brief Functia verifica daca un anumit document trebuie mutat sau nu
 *  @return Intoarce 1 daca documentul trebuie mutat, 0 in caz contrar
 */
int to_move(load_balancer *main, int server_id, int hash) {
	if (main->order[0].id_server == server_id) {
		/* Daca este primul server, cel cu cel mai mic hash */
		if (main->order[0].id_hash > hash)
			return 1;
		if (main->order[0].id_hash < hash && hash > main->order[1].id_hash)
			return 1;
		return 0;
	}

	int nr_servers = main->nr_servers;
	if (main->order[nr_servers - 1].id_server == server_id) {
		/* Daca este ultimul server, cel cu cel mai mare hash*/
		if (main->order[nr_servers - 1].id_hash > hash)
			if (main->order[0].id_hash < hash)
				return 1;
		return 0;
	}
	/* Am tratat cazurile de la capete ramane cazul general */

	int sourse_server_hash = main->hash_function_servers(&server_id);
	if (sourse_server_hash > hash)
		return 1;
	return 0;
}

/** @param sourse_server: serverul sursa
 *  @brief Functia executa coada de taskuri din serverul sursa
 */
void pop_queue(server *sourse_server) {
	request req;
	req.type = GET_DOCUMENT;
	/* Trimitem un request de GET pentru a da pop la coada de taskuri */
	req.doc_name = NULL;
	req.doc_content = NULL;
	/* Pentru a nu executa si functia de get care ar pusha pe cache
	 * un document care nu are ce cauta acolo o sa trimitem documentul NULL */

	server_handle_request(sourse_server, &req);
}

/** @param main: Load-balancerul
 *  @param server_id: Server id-ul
 *  @param cache_size: Dimensiunea cache-ului
 *  @brief Functia adauga un server pe hashring
 */
void loader_add_server(load_balancer* main, int server_id, int cache_size) {
	main->servers[server_id] = init_server(cache_size);
	main->servers[server_id]->server_id = server_id;
	main->nr_servers++;
	/* Cream serverul */

	main->order[main->nr_servers - 1].id_hash = hash_function_int(&server_id);
	main->order[main->nr_servers - 1].id_server = server_id;
	/* Il adaugam la final in lista de ordine */

	if (main->nr_servers == 1)
		return;
	/* Daca este un singur server nu mai facem nimic */

	int sourse_server_id = put_last_in_place(main->order, main->nr_servers);
	/* Punem serverul pe pozitia corespunzatoare in hashring si
	 * aflam id-ul serverului sursa */
	server *sourse_server = main->servers[sourse_server_id];
	pop_queue(sourse_server);
	/* Executam coada de taskuri din serverul sursa */

	hashtable_t *local_db = sourse_server->data_base;
	unsigned int nr_documente = ht_get_size(local_db);
	for (unsigned int i = 0; i < local_db->hmax && nr_documente; i++) {
		/* Iteram prin baza de date locala a serverului sursa
		 * cat inca avem documente */
		linked_list_t *bucket = local_db->buckets[i];
		ll_node_t *node = bucket->head;
		while (node) {
			int current_hash = main->hash_function_docs(((info *)node->data)->key);
			if (to_move(main, server_id, current_hash)) {
				/* Daca trebuie mutat documentul cu current_hash */
				ht_put(main->servers[server_id]->data_base,
					   ((info *)node->data)->key, DOC_NAME_LENGTH,
					   ((info *)node->data)->value, DOC_CONTENT_LENGTH);
				/* Adaugam documentul in serverul destinate */

				void *key = malloc(DOC_NAME_LENGTH);
                DIE(!key, "Eroare la alocare\n");

				memcpy(key, ((info *)node->data)->key, DOC_NAME_LENGTH);
				ll_node_t *aux = node->next;
				lru_cache_remove(sourse_server->cache, key);
				ht_remove_entry(local_db, key);
				/* Scoatem din baza de data, respectiv din cache daca e cazul */
				/* ceea ce se gaseste la cheia curenta */
				free(key);
				nr_documente--;
				node = aux;
			} else {
				node = node->next;
			}
		}
	}
}

/** @param order: Vectorul de ordine
 *  @param server_id: Id-ul serverului
 *  @param nr_servers: Nr total de servere
 *  @brief Functia pune la final in vectorul de ordine serverul sursa
 *  @return Returneaza id-ul serverului destinatie -1 la eroare
 */
int put_it_at_the_end(server_order *order, int server_id, int nr_servers) {
	if (order[nr_servers - 1].id_server == server_id)
		return order[0].id_server;
	/* Cazul de la margine */

	int dest_server_id;
	for (int i = 0; i < nr_servers; i++)
		if (order[i].id_server == server_id) {
			dest_server_id = order[i + 1].id_server;
			while (i < nr_servers - 1) {
				swap_servers_order(&order[i], &order[i + 1]);
				i++;
			}
			return dest_server_id;
		}
	return -1;
}

/** @param main: Load_balancerul
 *  @param server_id: Id-ul serverului
 *  @brief Functie elimina un server din hashring si ii elibereaza cu totul
 *		   memorie (cea consumata de continutul sau si de el insusi)
 */
void loader_remove_server(load_balancer* main, int server_id) {
	int dest_server_id = put_it_at_the_end(main->order, server_id,
										   main->nr_servers);
	/* Punem serverul pe care vrem sa il eliberam la final pe hashring
	 * si aflam id-ul serverului destinatie */
	main->nr_servers--;
	/* Micsoram nr de servere */

	pop_queue(main->servers[server_id]);
	/* Dam pop la coada de taskuri */

	server *sourse_server = main->servers[server_id];
	hashtable_t *local_db = sourse_server->data_base;

	unsigned int nr_documente = ht_get_size(local_db);
	for (unsigned int i = 0; i < local_db->hmax && nr_documente; i++) {
		/* Iteram prin tot local_data_baseul cat inca mai avem documente */
		linked_list_t *bucket = local_db->buckets[i];
		ll_node_t *node = bucket->head;
		while (node) {
			ht_put(main->servers[dest_server_id]->data_base,
				   ((info *)node->data)->key, DOC_NAME_LENGTH,
				   ((info *)node->data)->value, DOC_CONTENT_LENGTH);
			/* Adaugam documentul in data_baseul serverului destinate */

			nr_documente--;
			node = node->next;
		}
	}

	free_server(&main->servers[server_id]);
	/* Eliberam memoria ocupata de serverul scos */
}

/** @param order: Vectorul de ordine care o sa fie mereu sortat crescator
 *  @param hash: Hashul unui document
 *  @param nr_servers: Nr de servere
 *	@brief Cu ajutorul cautarii binare intoarcem id-ul serverului corespunzator
		   de pe hashring
	@return id-ul serverului care va primi requestul
 */
int kinda_binary_search(server_order *order, int hash, int nr_servers) {
	if (order[nr_servers - 1].id_hash < hash || order[0].id_hash > hash)
		return order[0].id_server;
	/* Situatia de la capete */

	int left = 0;
	int right = nr_servers - 1;
	int mij, index_to_find;

	while (left <= right) {
		mij = (left + right) / 2;

		if (order[mij].id_hash < hash) {
			left = mij + 1;
		} else {
			index_to_find = mij;
			right = mij - 1;
		}
	}

	return order[index_to_find].id_server;
}

/** @param main: load_balancer-ul
 *  @param req: request facut
 *  @brief Functia trimite un request de EDIT/GET spre serverul corespunzator
 * 		   de pe hashring
 *  @return Returneaza un raspuns
 */
response *loader_forward_request(load_balancer* main, request *req) {
	int hash = main->hash_function_docs(req->doc_name);
	int server_id = kinda_binary_search(main->order, hash, main->nr_servers);
	/* Gasim id-ul serverului care va prelua requestul */
	return server_handle_request(main->servers[server_id], req);
	/* Trimitem requestul */
}

/** @param main: load_balancerul
 *  @brief Functia elibereaza intreaga memorie ocupata de continutul
 *  	   load_balancer-ului si memoria ocupata de aceste
 */
void free_load_balancer(load_balancer** main) {
	load_balancer *to_remove = *main;

	for (int i = 0; i < to_remove->nr_servers; i++) {
		/* Parcurgem hashringul */
		int server_id = to_remove->order[i].id_server;
		free_server(&to_remove->servers[server_id]);
		/* Eliberam orice server care se afla pe hashring */
	}
	free(to_remove->servers);
	/* Eliberam memoria ocupata pentru vectorul de pointeri spre servere */
	free(to_remove->order);
	/* Eliberam memoria ocupata de Hashring */
	free(to_remove);
	/* Eliberam memoria ocupata de load_balancer */
	*main = NULL;
	/* Setam pe NULL */
}


