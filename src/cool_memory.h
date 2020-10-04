#include <stdlib.h>

typedef struct 
{
    ui8 *base;
    size_t size;
} GameMemory;

typedef struct
{
    ui8 *base;
    size_t used;
    size_t size;
    GameMemory *memory;
} MemoryArena;
