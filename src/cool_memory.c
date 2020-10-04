
MemoryArena *
CreateMemoryArena(size_t sizeInBytes)
{
    ui8 *totalMemory = malloc(sizeof(MemoryArena)+sizeInBytes);
    MemoryArena *arena = (MemoryArena *)totalMemory;
    arena->base = totalMemory + sizeof(MemoryArena);
    arena->used = 0;
    arena->size = sizeInBytes;
    arena->memory = NULL;
    return arena;
}

void *
PushMemory_(MemoryArena *arena, size_t size)
{
    arena->used+=size;
    Assert(arena->used < arena->size);
    return arena->base + arena->used - size;
}
#define PushStruct(arena, type) (type *)PushMemory_(arena, sizeof(type))
#define PushArray(arena, type, nElements) (type *)PushMemory_(arena, sizeof(type)*nElements)
