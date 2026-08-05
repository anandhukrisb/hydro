#pragma once

class ArenaAllocator {
public:
    inline ArenaAllocator(size_t bytes)
        :m_size(bytes)
    {
        m_buffer = malloc(m_size);
    }

    inline ArenaAllocator(const ArenaAllocator& other) = delete;

    inline ArenaAllocator operator = (const ArenaAllocator& other) = delete;

    inline ~ArenaAllocator() {
        free(m_buffer);
    }

private:
    size_t m_size;
    void* m_buffer;
};
