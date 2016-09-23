#include <stdexcept>
#include <string>

#include <iostream>

#include <list>
#include <memory>

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class AllocError: std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    AllocErrorType getType() const { return type; }
};

class Allocator;

class MemoryBlock {
    char *buffer;

    size_t offset;
    size_t length;
    bool state;

public:
    MemoryBlock(char *buffer_, size_t offset_, size_t length_) :
            buffer(buffer_), offset(offset_), length(length_), state(false) { }

    void set_offset(size_t offset_) { offset = offset_; }
    void set_length(size_t length_) { length = length_; }
    void set_state(bool state_) { state = state_; }

    size_t get_offset() const { return offset; }
    size_t get_length() const { return length; }
    bool is_free() const { return !state; }

    char *get_memory() const { return buffer + offset; }
};

class Pointer {
    MemoryBlock *p_block;

public:
    Pointer(MemoryBlock *p=NULL) : p_block(p) { }

    void *get() const { return p_block ? p_block->get_memory() : NULL; }

    MemoryBlock *get_block() const { return p_block; }
    void *set_block(MemoryBlock *p) { p_block = p; }
};

class Allocator {
    char *p_buffer;

    std::list<MemoryBlock> blocks;

public:
    Allocator(void *base, size_t size);
    
    Pointer alloc(size_t N);
    void realloc(Pointer &p, size_t N);
    void free(Pointer &p);

    void defrag();
    std::string dump();

private:
    void merge_empty_blocks();
    std::list<MemoryBlock>::iterator find_block (Pointer& p);
};

