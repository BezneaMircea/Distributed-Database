/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#ifndef SERVER_H
#define SERVER_H

#include "../utils/utils.h"
#include "../utils/constants.h"
#include "../cache/lru_cache.h"

#define TASK_QUEUE_SIZE         1000
#define MAX_LOG_LENGTH          1000
#define MAX_RESPONSE_LENGTH     4096

/** @param cache: memoria cache a serverului
 *  @param data_base: baza de date locala a serverului
 *  @param task_queue: coada de taskuri a serverului
 *  @param server_id: id-ul serverului
 */
typedef struct server {
	lru_cache *cache;
	hashtable_t *data_base;
	queue_t *task_queue;
	int server_id;
} server;

/** @param type: tipul de request
 *  @param doc_name: numele documentului
 *  @param doc_content: continutul documentului
 */
typedef struct request {
	request_type type;
	char *doc_name;
	char *doc_content;
} request;

/** @param server_log: raspunsul log al serverului
 *  @param server_response: raspunsul serverului
 *  @param server_id: id-ul serverului care raspunde
 */
typedef struct response {
	char *server_log;
	char *server_response;
	int server_id;
} response;

/** @param doc_name: numele documentului
 *  @param doc_content: continutul documentului
 *  @brief Acesta este data_typeul retinut de coada
 * 		   serverului
 */
typedef struct queue_data queue_data;
struct queue_data
{
    char doc_name[DOC_NAME_LENGTH];
    char doc_content[DOC_CONTENT_LENGTH];
};

server *init_server(unsigned int cache_size);

void free_server(server **s);

response *server_handle_request(server *s, request *req);

#endif  /* SERVER_H */
