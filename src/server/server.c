/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#include <stdio.h>
#include "server.h"
#include "../cache/lru_cache.h"

#include "../utils/utils.h"


/** @param s: serverul
 *  @param doc_name: numele documentului
 *  @param doc_content: continutul documentului
 *  @brief Functia executa un request de EDIT (care NU e lazy)
 *  @return Returneaza raspunsul adecvat
 */
static response
*server_edit_document(server *s, char *doc_name, char *doc_content) {
	response *resp = (response *)malloc(sizeof(response));
	DIE(!resp, "Eroare la alocare\n");

	resp->server_id = s->server_id;
	resp->server_log = (char *)malloc(MAX_LOG_LENGTH);
	DIE(!resp->server_log, "Eroare la alocare\n");

	resp->server_response = (char *)malloc(MAX_RESPONSE_LENGTH);
	DIE(!resp->server_response, "Eroare la alocare\n");
	/* Pregatim structura response */

	char *cache_content = lru_cache_get(s->cache, doc_name);
	/* Scoatem continutul asociat din cache */
	if (cache_content) {
		/* Daca este in cache */
		sprintf(resp->server_response, MSG_B, doc_name);
		sprintf(resp->server_log, LOG_HIT, doc_name);

		lru_cache_put(s->cache, doc_name, doc_content, NULL);
		ht_put(s->data_base, doc_name, DOC_NAME_LENGTH,
			   doc_content, DOC_CONTENT_LENGTH);
		/* Adaugam in local_db si in cache noul continut */
		return resp;
	}

	/* Nu este in cache */
	if (!ht_has_key(s->data_base, doc_name)) {
		/* Daca nu e nici in Local_data-base */
		sprintf(resp->server_response, MSG_C, doc_name);
	} else {
		/* Daca e in Local_data-base */
		sprintf(resp->server_response, MSG_B, doc_name);
	}
	ht_put(s->data_base, doc_name, DOC_NAME_LENGTH,
		   doc_content, DOC_CONTENT_LENGTH);
	/* Adaugam in Local_data-base noile date */

	void **evicted_key = malloc(sizeof(void *));
	DIE(!evicted_key, "Eroare la alocare\n");

	*evicted_key = malloc(DOC_NAME_LENGTH);
	DIE(!*evicted_key, "Eroare la alocare\n");

	lru_cache_put(s->cache, doc_name, doc_content, evicted_key);
	/* Facem schimbarea si in cache */

	if (!*evicted_key) {
		/* Nu a evacuat nicio cheie */
		sprintf(resp->server_log, LOG_MISS, doc_name);
		free(evicted_key);
		return resp;
	}

	/* A evacuat o cheie */
	sprintf(resp->server_log, LOG_EVICT, doc_name, (char *)*evicted_key);
	free(*evicted_key);
	free(evicted_key);
	return resp;
}

/** @param s: serverul
 *  @param doc_name: numele documentului
 *  @param doc_content: continutul documentului
 *  @brief Face editul lazy al documentului
 *  @return Returneaza raspunsul
 */
static response
*server_edit_document_lazy(server *s, char *doc_name, char *doc_content) {
	response *resp = (response *)malloc(sizeof(response));
	DIE(!resp, "Eroare la alocare\n");

	resp->server_log = (char *)malloc(MAX_LOG_LENGTH);
	DIE(!resp->server_log, "Eroare la alocare\n");

	resp->server_response = (char *)malloc(MAX_RESPONSE_LENGTH);
	DIE(!resp->server_log, "Eroare la alocare\n");

	resp->server_id = s->server_id;
	/* Am pregatit structura response */

	queue_data new_task;
	memcpy(new_task.doc_name, doc_name, DOC_NAME_LENGTH);
	memcpy(new_task.doc_content, doc_content, DOC_CONTENT_LENGTH);
	q_enqueue(s->task_queue, &new_task);
	/* Adaugam taskul in coada */

	sprintf(resp->server_log, LOG_LAZY_EXEC, q_get_size(s->task_queue));
	sprintf(resp->server_response, MSG_A, "EDIT", doc_name);
	/* Afisam mesajul corespunzator */

	return resp;
}

/** @param s: serverul
 *  @param doc_name: numele documentului pe care se da req de GET
 *  @brief Executa un request de GET doc_name pe serverul s
 *  @return Returneaza raspunsul serverului la requestul de GET
 */
static response
*server_get_document(server *s, char *doc_name) {
	if (!doc_name)
		return NULL;

	response *resp = (response *)malloc(sizeof(response));
	DIE(!resp, "Eroare la alocare\n");

	resp->server_id = s->server_id;
	resp->server_log = (char *)malloc(MAX_LOG_LENGTH);
	DIE(!resp->server_log, "Eroare la alocare\n");

	resp->server_response = (char *)malloc(MAX_RESPONSE_LENGTH);
	DIE(!resp->server_response, "Eroare la alocare\n");
	/* Alocam memorie pentru un raspuns */

	char *cache_content = lru_cache_get(s->cache, doc_name);
	/* Luam contentul din cache */
	if (cache_content) {
		/* Daca e in cache */
		strcpy(resp->server_response, cache_content);
		sprintf(resp->server_log, LOG_HIT, doc_name);

		return resp;
	}

	/* Nu e in cache */
	char *db_content = (char *)ht_get(s->data_base, doc_name);
	/* Luam contentul din Local_data-base*/
	if (!db_content) {
		/* Daca nu e in Local_data-base*/
		free(resp->server_response);
		resp->server_response = NULL;
		sprintf(resp->server_log, LOG_FAULT, doc_name);

		return resp;
	}

	/* E doar in local_data-base */
	void **evicted_key = malloc(sizeof(void *));
	DIE(!evicted_key, "Eroare la alocare\n");

	*evicted_key = malloc(DOC_NAME_LENGTH);
	DIE(!*evicted_key, "Eroare la alocare\n");

	lru_cache_put(s->cache, doc_name, db_content, evicted_key);
	/* Adaugam continutul in cache */
	if (!*evicted_key) {
		/* Daca nu a evacuat nicio cheie.
		 * Atentie, daca nu se evacueaza nicio cheie,
		 * funcita de put asigura eliberarea memorii alocate 
		 * pentru *evicted_key */
		sprintf(resp->server_log, LOG_MISS, doc_name);
		strcpy(resp->server_response, db_content);
		free(evicted_key);
		return resp;
	}
	/* A evacuat o cheie */
	sprintf(resp->server_log, LOG_EVICT, doc_name, (char *)*evicted_key);
	strcpy(resp->server_response, db_content);
	free(*evicted_key);
	free(evicted_key);
	return resp;
}

/** @param cache_size: Dimensiunea maxima a cacheului 
 *  @brief Functia care initializeaza un server
 * 		  avand ca dimensiune maxima a cacheului
 * 		  cache_size */
server *init_server(unsigned int cache_size) {
	server *new_server = malloc(sizeof(server));
	DIE(!new_server, "Eroare la alocare\n");

	new_server->cache = init_lru_cache(cache_size);
	new_server->data_base = ht_create(1000, hash_function_string,
									  compare_function_strings,
									  key_val_free_function);

	new_server->task_queue = q_create(sizeof(queue_data), TASK_QUEUE_SIZE);

	return new_server;
}

/** @param s: serverul
 *  @param req: requestul dat
 *  @brief Functia trateaza requesturile de tip EDIT si GET.
 * 		   Aceste requesturi sunt executate de serverul s.
 *  @return Returneaza raspunsul dat de server in urma requestului
 */
response *server_handle_request(server *s, request *req) {
	response *resp;

	if (req->type == EDIT_DOCUMENT)
		return server_edit_document_lazy(s, req->doc_name, req->doc_content);
	/* Daca avem un request de tip EDIT il executam lazy
	 * si intoarcem raspunsul */
	if (req->type == GET_DOCUMENT) {
	/* Trebuie sa executam coada de taskuri */
		while (!q_is_empty(s->task_queue)) {
			queue_data *q_data = (queue_data *)q_front(s->task_queue);
			resp = server_edit_document(s, q_data->doc_name, q_data->doc_content);
			/* Executam taskul curent si intoarcem raspunsul resp */
			PRINT_RESPONSE(resp);
			/* Afisam raspunsul */
			q_dequeue(s->task_queue);
		}
	}
	return server_get_document(s, req->doc_name);
	/* Intoarcem raspunsul pentru requestul de tip GET */
}

/** @param s: serverul
 *  @brief Functia elibereaza memoria ocupata de continutul serverului
 *		   si de serverul insusi.
 */
void free_server(server **s) {
	server *server_to_remove = *s;

	free_lru_cache(&server_to_remove->cache);
	/* Eliberam cache-ul */
	ht_free(server_to_remove->data_base);
	/* Eliveram loca_data-base */
	q_free(server_to_remove->task_queue);
	/* Eliberam memoria ocupata de continutul cozii */
	free(server_to_remove->task_queue);
	/* Eliberam coada */
	free(server_to_remove);
	/* Eliberam memoria ocupata de server */
	*s = NULL;
	/* Acum pointeaza spre NULL */
}
