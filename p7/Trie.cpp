#include "Trie.h"

Trie::Trie(const char *file_name)
{
    p_head = get_new_node(0);

    FILE * fd = fopen(file_name, "r");

    char buffer[64];
    while (fgets(buffer, 64, fd) > 0)
    {
        buffer[strlen(buffer)-1] = 0;
        add_new_word(buffer);
    }

    fclose(fd);
}

int Trie::get_words(const char *prefix, char **words)
{
    int n_words = 0;

    std::string word(prefix);
    word.pop_back();

    std::stack<Node *> nodes;
    std::stack<char> is_visited;

    auto p_node = get_node_prefix(prefix);
    nodes.push(p_node);
    is_visited.push(0);

    while (nodes.size())
    {
        char is_visited_ = is_visited.top();
        p_node = nodes.top();

        if (!is_visited_)
            word.push_back(p_node->c);

        is_visited.pop();
        is_visited.push(1);

        if (!is_visited_)
        {
            if (p_node->is_final)
            {
                words[n_words] = (char *) calloc(word.size() + 1, sizeof(char));
                memcpy(words[n_words], word.c_str(), word.size());

                if (++n_words == NUM_WORDS)
                    break;
            }

            for (size_t i = 0; i < NUM_LATIN_LETTERS; i++)
            {
                auto p_next = p_node->p_next[NUM_LATIN_LETTERS - 1 - i];
                if (p_next)
                {
                    nodes.push(p_next);
                    is_visited.push(0);
                }
            }

            if (p_node == nodes.top())
            {
                nodes.pop();
                is_visited.pop();
                word.pop_back();
            }
        }
        else
        {
            nodes.pop();
            is_visited.pop();
            word.pop_back();
        }
    }

    return n_words;
}

Trie::~Trie()
{
    std::stack<Node *> nodes;
    std::stack<char> is_visited;

    auto p_node = p_head;
    nodes.push(p_node);
    is_visited.push(0);

    while (nodes.size())
    {
        char is_visited_ = is_visited.top();
        p_node = nodes.top();

        is_visited.pop();
        is_visited.push(1);

        if (!is_visited_)
        {
            for (size_t i = 0; i < NUM_LATIN_LETTERS; i++)
            {
                auto p_next = p_node->p_next[NUM_LATIN_LETTERS - 1 - i];
                if (p_next)
                {
                    nodes.push(p_next);
                    is_visited.push(0);
                }
            }

            if (p_node == nodes.top())
            {
                delete p_node;
                nodes.pop();
                is_visited.pop();
            }
        }
        else
        {
            delete p_node;
            nodes.pop();
            is_visited.pop();
        }
    }
}

int Trie::get_char_index(char c)
{
    if ('a' <= c && c <= 'z')
    {
        return c - 'a';
    }
    else if ('A' <= c && c <= 'Z')
    {
        return c - 'A';
    }
    else
    {
        char buffer[256];
        sprintf(buffer, "There is symbol '%c' that is not supported", c);

        throw TrieError(TrieErrorType::NotAllowedSymbol, buffer);
    }
}

Node * Trie::get_new_node(char c)
{
    Node *p_node = new Node;
    p_node->is_final = 0;
    p_node->c = c;
    memset(p_node->p_next, 0, NUM_LATIN_LETTERS * sizeof(Node *));
    return p_node;
}

void Trie::add_new_word(const char *word)
{
    Node *p_node = p_head;

    for (size_t i = 0; i < strlen(word); i++)
    {
        int c_index = get_char_index(word[i]);

        if (p_node->p_next[c_index] == NULL)
            p_node->p_next[c_index] = get_new_node(word[i]);

        p_node = p_node->p_next[c_index];
    }
    p_node->is_final = 1;
}

Node* Trie::get_node_prefix(const char *prefix)
{
    Node *p_node = p_head;

    for (size_t i = 0; i < strlen(prefix); i++)
    {
        int c_index = get_char_index(prefix[i]);

        if (p_node->p_next[c_index])
        {
            p_node = p_node->p_next[c_index];
        }
        else
        {
            throw TrieError(
                    TrieErrorType::NoPrefix,
                    "There are no words with such prefix"
            );
        }
    }

    return p_node;
}