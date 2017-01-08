#ifndef P5_HASHTABLE_H
#define P5_HASHTABLE_H

#include <iostream>
#include <cstring>

enum class HashTableErrorType
{
    NoKey,
    NoMemory,
};

class HashTableError: public std::runtime_error
{
private:
    HashTableErrorType type;

public:
    HashTableError(HashTableErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    HashTableErrorType getType() const { return type; }
};

struct Node
{
    int key, value;
    int n_prev, n_next;
};

class NodePtr
{
public:
    NodePtr(Node *base, int n_node) : p_base(base) { p_node = (n_node != -1) ? base + n_node : NULL; }

    Node* get_ptr() { return p_node; }
    int   get_num() { return get_num(*this); }

    void set_key(int key) { p_node->key = key; }
    void set_value(int value) { p_node->value = value; }

    int get_key() const { return p_node->key; }
    int get_value() const { return p_node->value; }

    void set_next(int n_node) { if (p_node) p_node->n_next = n_node; }
    void set_next(NodePtr ptr) { if (p_node) p_node->n_next = get_num(ptr); }

    NodePtr get_next() { return (p_node) ? NodePtr(p_base, p_node->n_next) : NodePtr(p_base, -1); }

    void set_prev(int n_node) { if (p_node) p_node->n_prev = n_node; }
    void set_prev(NodePtr ptr) { if (p_node) p_node->n_prev = get_num(ptr); }

    NodePtr get_prev() { return (p_node) ? NodePtr(p_base, p_node->n_prev) : NodePtr(p_base, -1); }

    bool operator ==(Node * ptr) { return (ptr == p_node); }
    bool operator ==(NodePtr *p) { return (p->p_node == p_node);}

    bool operator !=(Node * ptr) { return (ptr != p_node); }
    bool operator !=(NodePtr *p) { return (p->p_node != p_node);}

    bool is_null() { return (p_node == NULL); }

private:
    int get_num(NodePtr ptr) { return (ptr.p_node) ? static_cast<int>(ptr.p_node - p_base) : -1; }

private:
    Node *p_base, *p_node;
};

class HashTable
{
public:
    HashTable(void *shmaddr);

    void set(int key, int value);
    int  get(int key) const;
    void del(int key);

    void dump() const;

    ~HashTable() { }

    int get_hash(int key) const { return static_cast<int>(std::hash<int>()(key) % HASH_TABLE_SIZE); }

private:
    int get_num_free_node() const { return p_table[HASH_TABLE_SIZE]; }
    void set_num_free_node(int k) { p_table[HASH_TABLE_SIZE] = k; }

    NodePtr find_node(int key) const;

    NodePtr get_node_free();
    void set_node_free(NodePtr p_node);

public:
    static const int NUM_SECTIONS = 8;

    static const size_t HASH_TABLE_SIZE = 256;
    static const size_t BUFFER_SIZE = 2048;

private:
    void *p_mem;
    Node *p_data;
    int  *p_table;
};


#endif //P5_HASHTABLE_H
