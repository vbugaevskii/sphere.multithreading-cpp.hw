#include <cstring>
#include "allocator.h"

Allocator::Allocator(void *base, size_t size)
{
    p_buffer = static_cast<char *>(base);
    blocks.push_back(MemoryBlock(p_buffer, 0, size));
}

Pointer Allocator::alloc(size_t N)
{
    bool is_allocated = false;
    auto it = blocks.begin();

    for ( ; it != blocks.end(); it++)
    {
        if (!it->is_free())
            continue;

        size_t block_length = it->get_length();
        size_t block_offset = it->get_offset();

        if (block_length >= N)
        {
            it->set_state(true);

            if (block_length > N)
            {
                it->set_length(N);
                // should return the iterator for allocated block
                blocks.insert((++it)--, MemoryBlock(p_buffer, block_offset + N, block_length - N));
            }

            is_allocated = true;
        }

        if (is_allocated)
            break;
    }

    if (!is_allocated)
    {
        std::string msg = "ERROR in alloc, no memory!\n";
        throw AllocError(AllocErrorType::NoMemory, msg);
    }

    return Pointer(&(*it));
}

void Allocator::defrag()
{
    size_t block_length = 0;        // this is sum of free blocks' lengths
    size_t block_offset = 0;        // new offset of a non-free block

    for (auto it = blocks.begin(); it != blocks.end(); it++)
    {
        if (it->is_free())
        {
            block_length += it->get_length();
            blocks.erase(it--);
        }
        else
        {
            size_t offset = it->get_offset();

            it->set_offset(block_offset);
            block_offset += offset;

            // move memory
            memmove(p_buffer + block_offset, p_buffer + offset, it->get_length());
        }
    }

    blocks.push_back(MemoryBlock(p_buffer, block_offset, block_length));
}

void Allocator::realloc(Pointer &p, size_t N)
{
    if (p.get() == NULL)
    {
        p = alloc(N);
        return ;
    }

    // info about current block
    auto it_block = find_block(p);
    size_t offset = it_block->get_offset();
    size_t length = it_block->get_length();

    if (length == N)            // nothing changes
        return ;
    else if (length > N)        // just shrink the block
    {
        it_block->set_length(N);
        blocks.insert(++it_block, MemoryBlock(p_buffer, offset + N, length - N));
    }
    else
    {
        size_t block_length = 0;            // length of a candidate block
        auto it_block_new = blocks.end();   // candidate block

        for (auto it = blocks.begin(); it != blocks.end(); it++)
        {
            if (it->is_free() || it == it_block)
            {
                if (!block_length)
                    it_block_new = it;

                block_length += it->get_length();
            }
            else
            {
                if (block_length >= N)
                    break;

                it_block_new = blocks.end();
                block_length = 0;
            }
        }

        if (block_length < N)
        {
            std::string msg = "ERROR in realloc, no memory!\n";
            throw AllocError(AllocErrorType::NoMemory, msg);
        }

        // erase merged blocks
        for (auto it = it_block_new; it != blocks.end(); it++)
        {
            if (it == it_block_new)
                continue;
            else if (it->is_free() || it == it_block)
                blocks.erase(it--);
        }

        // set info for a candidate block
        size_t block_offset = it_block_new->get_offset();
        it_block_new->set_length(N);
        it_block_new->set_state(true);
        p.set_block(&(*it_block_new));

        it_block->set_state(false);

        blocks.insert(++it_block_new, MemoryBlock(p_buffer, block_offset + N, block_length - N));

        // move memory
        memmove(p_buffer + block_offset, p_buffer + offset, length);
    }

    merge_empty_blocks();
}

void Allocator::free(Pointer& p)
{
    MemoryBlock *p_block = p.get_block();

    if (!p_block)
    {
        std::string msg = "ERROR in free, pointer is NULL!\n";
        throw AllocError(AllocErrorType::InvalidFree, msg);
    }

    p_block->set_state(false);

    p.set_block(NULL);

    merge_empty_blocks();
}

void Allocator::merge_empty_blocks()
{
    MemoryBlock *p_block = NULL;
    size_t length_total = 0;

    for (auto it = blocks.begin(); it != blocks.end(); it++)
    {
        if (it->is_free())
        {
            length_total += it->get_length();

            if (length_total == it->get_length())
                p_block = &(*it);
            else
            {
                blocks.erase(it);
                it--;
            }

        }
        else
        {
            if (p_block)
                p_block->set_length(length_total);

            p_block = NULL;
            length_total = 0;
        }
    }

    if (p_block)
        p_block->set_length(length_total);
}

std::list<MemoryBlock>::iterator Allocator::find_block (Pointer& p)
{
    size_t offset = p.get_block()->get_offset();
    auto block = blocks.begin();
    for ( ; block != blocks.end(); block++)
    {
        if (block->get_offset() == offset)
            break;
    }
    return block;
}

std::string Allocator::dump()
{
    char msg_dump[128];
    std::string msg = "";

    for (auto block : blocks) {
        sprintf(msg_dump, "offset: %zu; ", block.get_offset());
        msg += std::string(msg_dump);
        sprintf(msg_dump, "length: %zu; ", block.get_length());
        msg += std::string(msg_dump);
        sprintf(msg_dump, "empty: %s\n", block.is_free() ? "True" : "False");
        msg += std::string(msg_dump);
    }

    return msg;
}