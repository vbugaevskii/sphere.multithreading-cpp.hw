#ifndef P7_TRIE_H
#define P7_TRIE_H

#include <stack>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

enum class TrieErrorType
{
    NoPrefix,
    NotAllowedSymbol
};

class TrieError: public std::runtime_error
{
private:
    TrieErrorType type;

public:
    TrieError(TrieErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    TrieErrorType getType() const { return type; }
};

#define NUM_LATIN_LETTERS 26

struct Node
{
    Node *p_next[NUM_LATIN_LETTERS];
    char is_final, c;
};

class Trie
{
public:
    Trie() { p_head = get_new_node(0); }
    Trie(const char *file_name);

    int get_words(const char *prefix, char **words);

    ~Trie();

private:
    int get_char_index(char c);

    Node *get_new_node(char c);

    void add_new_word(const char *word);

    Node* get_node_prefix(const char *prefix);

public:
    static const size_t NUM_WORDS = 10;

private:
    Node *p_head;
};


#endif //P7_TRIE_H
