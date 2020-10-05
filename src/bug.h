typedef struct BugLoop BugLoop;
typedef struct Bug Bug;
struct BugLoop
{
    Vec3 pos;
    r32 radius;
    r32 speedFactor;
    int nBugs;
    Bug **bugs;
};

struct Bug
{
    r32 orientation;
    Vec3 pos;
    r32 zVel;
    r32 scale;
    Vec3 feetFrom[6];
    Vec3 feetTo[6];
    int loopNumber;
};

typedef struct
{
    r32 width;
    r32 height;
    r32 aiSpeed;
    int nBugs;
    int maxBugs;
    Bug *bugs;

    b32 isLoopDistributionDirty;
    int nLoops;
    int maxLoops;
    BugLoop *loops;
    Bug **loopBugPointers;

    Vec3 loopColors[8];
} World;
