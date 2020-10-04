typedef struct
{
    Vec3 pos;
    Vec3 oldPos;
} Verlet;

typedef struct
{
    real32 r;
    Verlet *a;
    Verlet *b;
} Constraint;


