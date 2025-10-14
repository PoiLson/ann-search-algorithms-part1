struct LSH; 

struct hash_table;

struct node
{
    void *key;
    void *data;
    struct node *next;
};

typedef struct node *Node;
typedef struct hash_table *HashTable;

#define MAP_EOF (Node)0
#define MAP_BOF (Node)0

typedef void (*funtion)(void *);

typedef int (*Compare_fun)(void *, void *);

typedef int (*Hash_fun)(HashTable, void *);

// finds the prime number before a given number
int find_prime(int number);

// creates a hash table with a given capacity
HashTable hash_table_create(int capacity, funtion destroy, Compare_fun compare, Hash_fun hash_function);

// inserts a key and data in the hash table
int hash_table_insert(HashTable hash_table, void *key, void *data);

// searches for a key in the hash table
void *hash_table_search(HashTable hash_table, void *key);

// destroys the hash table
void hash_table_destroy(HashTable hash_table);

// returns the number of elements in the hash table
int hash_table_size(HashTable hash_table);

// removes the element with the given key from the hash table
int hash_table_remove(HashTable hash_table, void *key);

// returns the capacity of the hash table
int hash_table_capacity(HashTable hash_table); 

// returns the head of the linked list at the given index
Node hash_table_get_bucket(HashTable hash_table, int index);

void print_hashtables(const struct LSH* lsh, int dimension);