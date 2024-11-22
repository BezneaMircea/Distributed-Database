/*
 * Copyright (c) 2024, <> Beznea Mircea <bezneamirceaandrei21@gmail.com>
 */

#include "structuri.h"


/* Functia creeaza o lista dublu inlantuita */
dll_list *dll_create(int data_size) {
	dll_list *lista = malloc(sizeof(dll_list));
    DIE(!lista, "Eroare la alocare\n");

	lista->data_size = data_size;
	lista->head = NULL;
	lista->size = 0;

	return lista;
}

/* Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Functia intoarce un pointer spre acest nod
 * proaspat eliminat din lista. Daca n >= nr_noduri - 1, se elimina nodul de la
 * finalul listei. Este responsabilitatea apelantului sa elibereze memoria
 * acestui nod.
 * Atentie: n>=0 (nu trebuie tratat cazul in care n este negativ).*/
dll_node *dll_remove_nth_node(dll_list *list, int n) {
	dll_node *ptr;
	if (list->size == 1) {
		ptr = list->head;
		list->head = NULL;
		list->size--;
		return ptr;
	}

	if (n >= list->size)
		n = list->size - 1;

	ptr = list->head;
	for (int i = 0; i < n; i++)
		ptr = ptr->next;
	ptr->prev->next = ptr->next;
	ptr->next->prev = ptr->prev;
	list->size--;
	if (n == 0)
		list->head = ptr->next;
	return ptr;
}

/* Adauga un nod continand informatia new_data in lista 
 * dublu inlantuita. Indexarea se face de la 0 (primului
 * element ii corespune 1 s.a.s.m.d.) */
void
dll_add_nth_node(dll_list *list, int n, void *new_data) {
	if (!list) {
		printf("Create a list first\n");
		return;
	}

	dll_node *nod = malloc(sizeof(dll_node));
    DIE(!nod, "Eroare la alocare\n");

	nod->data = malloc(list->data_size);
    DIE(!nod->data, "Eroare la alocare\n");

	memcpy(nod->data, new_data, list->data_size);
	nod->next = NULL;
	nod->prev = NULL;
	/* Avem nodul ce trebuie adaugat */

	if (n >= list->size)
		n = list->size;

	dll_node *aux;
	if (list->size == 0) { /* Daca lista e goala */
		list->head = nod;
		list->head->prev = nod;
		list->head->next = nod;
	} else if (n == 0) { /* Daca adaugam la inceput */
		aux = list->head;
		list->head = nod;
		nod->next = aux;
		nod->prev = aux->prev;
		aux->prev = list->head;
		list->head->prev->next = list->head;
	} else {
		dll_node *ptr;
		ptr = list->head;

		for (int i = 0; i < n - 1; i++)
			ptr = ptr->next;

		aux = ptr->next;
		ptr->next = nod;
		nod->prev = ptr;
		nod->next = aux;
		nod->next->prev = nod;
	}

	list->size++;
}

/* Functia intoarce dimensiunea listei */
int dll_get_size(dll_list *list) {
	return list->size;
}


void
dll_print_int_list(dll_list* list) {
	dll_node *ptr = list->head;

	for (int i = 0; i < list->size; i++) {
		printf("%d ", *(int *)ptr->data);
		ptr = ptr->next;
	}
	printf("\n");
}


void
dll_print_string_list(dll_list* list) {
	if (!list)
		return;

	dll_node *ptr = list->head;

	for (int i = 0; i < list->size; i++) {
		printf("%s ", (char *)ptr->data);
		ptr = ptr->next;
	}
	printf("\n");
}

/* Elibereza toata memoria folosita de lista.
 * Al doilea parametru este o functie care elibereaza 
 * memoria ocupata de campul data al unui nod in cazul
 * in care pastram structuri complexe in nodurile listei.
 * Daca al doilea parametru este NULL, se elibereaza doar memoria
 * ocupata de nodurile liste si de lista. La final lista este setata 
 * pe NULL */ 
void
dll_free(dll_list **pp_list, void (*elem_free)(void *)) {
	dll_list *list = *pp_list;

	if (!list)
		return;

	if (!list->head || !list->size) {
		free(list);
		*pp_list = NULL;
		return;
	}

	dll_node *tmp, *ptr;
	tmp = list->head;
	ptr = list->head->next;

	for (int i = 0; i < list->size; i++) {
		if (elem_free)
			elem_free(tmp->data);
		free(tmp->data);
		free(tmp);

		tmp = ptr;
		if (i < list->size - 1)
			ptr = ptr->next;
	}
	free(list);
	*pp_list = NULL;
}


/* Elimina in O(1) ultimul nod al listei
 * memoria este eliberata de apelant*/
dll_node *dll_remove_nth_node_from_end(dll_list *list) {
	if (!list)
		return NULL;

	if (list->size == 0)
		return NULL;

	dll_node *nod_to_remove = list->head->prev;
	/* ultimul nod */
	nod_to_remove->prev->next = nod_to_remove->next;
	nod_to_remove->next->prev = nod_to_remove->prev;

	list->size--;

	if (list->size == 0)
		list->head = NULL;

	return nod_to_remove;
}


/* Functia scoate din lista nodul spre care pointeaza node_to_remove 
 * si elibereaza memoria consumata de acesta.
 * Functia se executa in O(1) */
void remove_a_node(dll_node *node_to_remove, dll_list *list) {
	if (!node_to_remove)
		return;

	if (list->size == 1 && node_to_remove == list->head) {
		/* Daca eliberam doar primul nod din lista si dupa 
		 * lista va ramane goala */
		dll_node *ptr = list->head;
		free(ptr->data);
		free(ptr);

		list->head = NULL;
		list->size--;
		return;
	}

	if (node_to_remove == list->head) {
		/* Daca elibera primul nod din lista */
	 	dll_node *tail = list->head->prev;
	 	list->head->prev->next = list->head->next;
	 	list->head->next->prev = list->head->prev;
	 	free(list->head->data);
	 	free(list->head);

		list->head = tail->next;
		list->size--;
	 	return;
	}

	/* Orice alta situatie */
	node_to_remove->prev->next = node_to_remove->next;
	node_to_remove->next->prev = node_to_remove->prev;

	free(node_to_remove->data);
	free(node_to_remove);
	node_to_remove = NULL;
	list->size--;
}
