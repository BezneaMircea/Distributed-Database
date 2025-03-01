# Beznea Mircea-Andrei

## TOPIC 2 - Distributed Database

---

### DESCRIPTION:

#### Cache Implementation:
We will store both the document content and a pointer to the corresponding node in the ordered list in the hashtable.  
The ordered list will only store the keys (the document names).

In addition to the functions of the HashMap, the constant complexity of this implementation is ensured by two additional functions on a circular doubly linked list: `remove_a_node` and `dll_remove_nth_node_from_the_end`.

#### Server Implementation:
The architecture with 3 components will consist of:
- A queue from `queue.c`
- A cache
- A HashMap that will represent the Local Data Base

The server structure will also include its unique ID as an integer.

#### Load Balancer Implementation:
The load balancer will maintain:
- An array of pointers to servers
- An order array that holds structures with two fields: `server_id` and `server_hash`
- Functions to perform hashing both on servers (int) and on documents (char *)

In terms of complexity:
- Finding the server that will handle a request will be done in `O(log n)`
- The remaining functionalities will be executed in `O(n)`

---

### TESTING:

To test, run the following commands in the Linux CLI from the `src` directory:

```bash
make
./program ../in/test$(testNr).in  
The actual checker is not included in this repository
