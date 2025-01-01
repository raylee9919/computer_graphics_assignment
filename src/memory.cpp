/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Memory_Arena
{
    u8 *base;
    size_t size;
    size_t used;
};

static void 
init_arena(Memory_Arena *arena, void *_base, size_t _size)
{
    arena->base = (u8 *)_base;
    arena->size = _size;
    arena->used = 0;
}

#define push_array(ARENA, STRUCT, COUNT) (STRUCT *)push_size(ARENA, sizeof(STRUCT)*COUNT)
#define push_struct(ARENA, STRUCT) (STRUCT *)push_size(ARENA, sizeof(STRUCT))
static void *push_size(Memory_Arena *arena, size_t size)
{
    void *result;
    ASSERT(arena->used + size <= arena->size);
    result = arena->base + arena->used;
    arena->used += size;
    return result;
}
