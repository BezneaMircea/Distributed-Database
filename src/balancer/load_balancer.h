/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "../server/server.h"

#define MAX_SERVERS             99999


/** @param id_server: id_ul serverului
 *  @param id_hash: hashul serverului care are id-ul id_server
 */
typedef struct server_order {
	int id_server;
	int id_hash;
} server_order;

/** @param hash_function_server: Pointer spre o functie de hashing pt servere
 *  @param hash_function_docs: Pointer spre o functie de hashing pt documente
 *  @param servers: Vector de pointeri spre servere
 *  @param order: Vector de ordine
 *  @param nr_servers: Nr de servere
 */
typedef struct load_balancer {
	unsigned int (*hash_function_servers)(void *);
	unsigned int (*hash_function_docs)(void *);

	server **servers;
	server_order *order;
	int nr_servers;
} load_balancer;

load_balancer *init_load_balancer(bool enable_vnodes);

void free_load_balancer(load_balancer** main);

void loader_add_server(load_balancer* main, int server_id, int cache_size);

void loader_remove_server(load_balancer* main, int server_id);

response *loader_forward_request(load_balancer* main, request *req);


#endif /* LOAD_BALANCER_H */
