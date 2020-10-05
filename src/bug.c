
internal inline void
CalculateFeetPos(Vec3 center, Vec3 direction, Vec3 feet[2])
{
    feet[0] = vec3(center.x - direction.y, center.y+direction.x, center.z);
    feet[1] = vec3(center.x + direction.y, center.y-direction.x, center.z);
}

internal inline Vec3
GetBugCenter(int nBugs, Bug *bugs[nBugs])
{
    Vec3 center = vec3(0,0,0);
    for(int bugIdx = 0;
            bugIdx < nBugs;
            bugIdx++)
    {
        center = v3_add(center, bugs[bugIdx]->pos);
    }
    return v3_muls(center, 1.0/nBugs);
}

internal inline void
SortBugsIntoLoops(World *world)
{
    int nBugsInLoop[world->nLoops];
    int cumulativeBugs = 0;
    memset(nBugsInLoop, 0, sizeof(nBugsInLoop));
    for(int bugIdx = 0;
            bugIdx < world->nBugs;
            bugIdx++)
    {
        Bug *bug = world->bugs+bugIdx;
        nBugsInLoop[bug->loopNumber]++;
    }
    for(int loopIdx = 0;
            loopIdx < world->nLoops;
            loopIdx++)
    {
        BugLoop *loop = world->loops+loopIdx;
        loop->nBugs = 0;
        loop->bugs = world->loopBugPointers+cumulativeBugs;
        cumulativeBugs+=nBugsInLoop[loopIdx];
    }
    for(int bugIdx = 0;
            bugIdx < world->nBugs;
            bugIdx++)
    {
        Bug *bug = world->bugs+bugIdx;
        BugLoop *loop = world->loops+bug->loopNumber;
        loop->bugs[loop->nBugs++] = bug;
    }
}

internal inline void
MoveBugToLoop(World *world, Bug *bug, int loopNumber)
{
    bug->loopNumber = loopNumber;
    bug->zVel=1;
    bug->scale+=0.25;
    world->isLoopDistributionDirty = 1;
}

internal inline BugLoop*
AddLoop(MemoryArena *arena, World *world)
{
    BugLoop *loop = world->loops + world->nLoops++;
    Assert(world->nLoops <= world->maxLoops);
    loop->pos = vec3(RandomFloat(0, world->width), RandomFloat(0, world->height), 0);
    loop->radius = 20;
    loop->speedFactor = RandomFloat(0.8, 1.0);
    loop->bugs = PushArray(arena, Bug*, world->maxBugs);
    loop->nBugs = 0;
    return loop;
}

internal inline Bug*
AddBug(World *world, int loopNumber)
{
    Bug *bug = world->bugs + world->nBugs++;
    Assert(world->nBugs <= world->maxBugs);
    BugLoop *loop = world->loops+loopNumber;
    bug->scale = 1.0;
    bug->pos = vec3(loop->pos.x, loop->pos.y, 1.0);
    bug->zVel = 1;
    bug->loopNumber = loopNumber;
    return bug;
}

internal inline void
UpdateAndRenderLoops(World *world, Mesh *mesh)
{
    if(world->isLoopDistributionDirty)
    {
        world->isLoopDistributionDirty = 0;
        SortBugsIntoLoops(world);
    }
    for(int loopIdx = 0;
            loopIdx < world->nLoops;
            loopIdx++)
    {
        BugLoop *loop = world->loops + loopIdx;
        mesh->colorState = world->loopColors[loopIdx%8];
        PushLineCircle(mesh, v3_add(loop->pos, vec3(0,0,0.1)), loop->radius, 20, 0.1);
        if(loop->nBugs > 0)
        {
            loop->radius = 2*sqrtf(loop->nBugs);
            Vec3 center = GetBugCenter(loop->nBugs, loop->bugs);
            loop->pos = lerp(center, loop->pos, 0.95);
        }
        else
        {
            loop->radius = 1;
        }

        if(loopIdx!=0)
        {
            // Do loop ai. Check for closest loop. If bigger then move away. 
            // If smaller move towards
            r32 aiSpeed = world->aiSpeed * loop->speedFactor;
            r32 minDist = 1000000.0;
            BugLoop *nearestEnemy = NULL;
            Vec3 nearestDiff;

            for(int enemyIdx = 0;
                    enemyIdx < world->nLoops;
                    enemyIdx++)
            {
                if(enemyIdx!=loopIdx)
                {
                    BugLoop *enemy = world->loops+enemyIdx;
                    if(enemy->nBugs > 0)
                    {
                        Vec3 diff = v3_sub(enemy->pos, loop->pos);
                        r32 dist = v3_length(diff);
                        if(dist < minDist)
                        {
                            minDist = dist;
                            nearestEnemy = enemy;
                            nearestDiff = diff;
                        }
                    }
                }
            }
            if(nearestEnemy!=NULL && minDist > loop->radius/2)
            {
                nearestDiff = v3_muls(v3_norm(nearestDiff), aiSpeed);
                if(nearestEnemy->nBugs < loop->nBugs)
                {
                    loop->pos = v3_add(loop->pos, nearestDiff);
                }
                else
                {
                    loop->pos = v3_sub(loop->pos, nearestDiff);
                }
            }
            // find nearest edge and move away from it
            r32 edgeFactor = 2.0;
            r32 minEdgeDist = 100.0;
            r32 x0 = loop->pos.x;
            r32 x1 = world->width - loop->pos.x;
            r32 y0 = loop->pos.y;
            r32 y1 = world->height - loop->pos.y;
            r32 min = 1000000.0;
            int xDir = 0;
            int yDir = 0;
            if(x0 < min) {min = x0; xDir=1; yDir=0;}
            if(x1 < min) {min = x1; xDir=-1; yDir=0;}
            if(y0 < min) {min = y0; xDir=0; yDir=1;}
            if(y1 < min) {min = y1; xDir=0; yDir=-1;}
            if(min < minEdgeDist)
            {
                r32 e = (minEdgeDist-min)*edgeFactor/minEdgeDist;
                loop->pos = v3_add(loop->pos, vec3(xDir*e, yDir*e, 0));
            }
        }
    }
}

internal inline void
CollideLoops(World *world, BugLoop *loopA, BugLoop *loopB)
{
    int total = loopA->nBugs+loopB->nBugs;
    r32 probA = ((r32)loopA->nBugs)/((r32)total);
    for(int bug0Idx = 0; 
            bug0Idx < loopA->nBugs;
            bug0Idx++)
    {
        Bug *bug0 = loopA->bugs[bug0Idx];
        for(int bug1Idx = 0;
                bug1Idx < loopB->nBugs;
                bug1Idx++)
        {
            Bug *bug1 = loopB->bugs[bug1Idx];
            r32 dx = bug1->pos.x-bug0->pos.x;
            r32 dy = bug1->pos.y-bug0->pos.y;
            r32 len2 = dx*dx + dy*dy;
            if(len2 < 4)
            {
                if(RandomFloat(0.0, 1.0) < 0.1)
                {
                    if(RandomFloat(0.0, 1.0) < probA)
                    {
                        MoveBugToLoop(world, bug1, bug0->loopNumber);
                    }
                    else
                    {
                        MoveBugToLoop(world, bug0, bug1->loopNumber);
                    }
                }
            }
        }
    }
}

internal inline void
UpdateAndRenderBugs(World *world, Mesh *mesh)
{
    // Render dynamic
    local_persist r32 time = 0;
    time += 1.0/60;
    r32 speedFactor = 0.6;
    for(int bugIdx = 0;
            bugIdx < world->nBugs;
            bugIdx++)
    {
        Bug *bug = world->bugs+bugIdx;
        mesh->colorState = world->loopColors[bug->loopNumber%8];
        r32 speed = speedFactor * RandomFloat(0, 1);

        // Loop calculations
        BugLoop *loop = world->loops + bug->loopNumber;
        r32 loopInfluence = 0.14;
        Vec3 loopDiff = v3_sub(bug->pos, loop->pos);
        r32 loopDist = v3_length(loopDiff);
        r32 distToLoop = loopDist-loop->radius;
        r32 absDistToLoop = distToLoop < 0.0 ? -distToLoop : distToLoop;
        if(absDistToLoop > 10.0)
        {
            speed*=2;
        }

        r32 c = sinf(bug->orientation);
        r32 s = cosf(bug->orientation);
        bug->zVel-=0.05;
        bug->pos = v3_add(bug->pos, vec3(c*speed, s*speed, 0));
        bug->pos.z+=bug->zVel;
        if(bug->pos.z < 1.0)
        {
            bug->pos.z = 1.00;
            bug->zVel*=-0.8;
        }

        // See if loop center is left or right of bug. Move accordingly
        r32 perpDot = -loopDiff.x*s + loopDiff.y*c;
        if(perpDot > 0)
        {
            if(distToLoop > 0)
            {
                bug->orientation += RandomFloat(0, loopInfluence);
            }
            else
            {
                bug->orientation -= RandomFloat(0, loopInfluence);
            }
        }
        else
        {
            if(distToLoop > 0)
            {
                bug->orientation -= RandomFloat(0, loopInfluence);
            }
            else
            {
                bug->orientation += RandomFloat(0, loopInfluence);
            }
        }

        // Bound bug
        if(bug->pos.x < 0)
        {
            bug->pos.x = 0;
        }
        if(bug->pos.y < 0)
        {
            bug->pos.y=0;
        }
        if(bug->pos.x > world->width)
        {
            bug->pos.x = world->width;
        }
        if(bug->pos.y > world->height)
        {
            bug->pos.y = world->height;
        }

        // Draw Body
        r32 scale = bug->scale;
        r32 lineWidth = scale*0.06;
        Vec3 from = bug->pos;
        Vec3 to = v3_add(from, vec3(c*scale, s*scale, 0));
        PushTrapezoid(mesh, from, to, 0.7*scale, 0.3*scale, vec3(0,0,1));

        // Draw antenna
        r32 antennaTheta = time * 6;
        r32 antennaMovement = 0.3;
        r32 antCos = cosf(antennaTheta)*antennaMovement*scale;
        r32 antSin = sinf(antennaTheta)*antennaMovement*scale;
        Vec3 antenna0 = v3_add(to, vec3(-scale*s*0.5 + antCos, scale*c*0.5+antSin, scale));
        Vec3 antenna1 = v3_add(to, vec3(scale*s*0.5 -antSin, -scale*c*0.5+antCos, scale));
        PushLine(mesh, to, antenna0, lineWidth, vec3(c,s,0));
        PushLine(mesh, to, antenna1, lineWidth, vec3(c,s,0));

        // Draw and update feet
        Vec3 feet[6];
        Vec3 groundFrom = from;
        Vec3 groundTo = to;
        groundFrom.z = 0;
        groundTo.z = 0;
        Vec3 direction = vec3(c, s, 0);
        
        for(int feetPairIdx = 0; 
                feetPairIdx < 3; 
                feetPairIdx++)
        {
            Vec3 bodyPos = lerp(from, to, 0.5*feetPairIdx);
            bug->feetFrom[feetPairIdx*2] = bodyPos;
            bug->feetFrom[feetPairIdx*2+1] = bodyPos;
            Vec3 center = lerp(groundFrom, groundTo, 0.5*feetPairIdx+0.5); // A little ahead
            center.z = bodyPos.z-1.0;
            CalculateFeetPos(center, direction, feet+feetPairIdx*2); 
        }

        for(int footIdx = 0;
                footIdx < 6;
                footIdx++)
        {
            Vec3 newFootPos = feet[footIdx];
            Vec3 footPos = bug->feetTo[footIdx];
            r32 diff = v3_length(v3_sub(newFootPos, footPos));
            if(diff > 1)
            {
                bug->feetTo[footIdx] = vec3(newFootPos.x + RandomFloat(-0.2, 0.2), 
                        newFootPos.y+RandomFloat(-0.2, 0.2), 
                        newFootPos.z);
            }
            PushLine(mesh, bug->feetFrom[footIdx], bug->feetTo[footIdx], lineWidth, vec3(0,0,1));
        }
    }
#if 1
    for(int loopAIdx = 0;
            loopAIdx < world->nLoops -1;
            loopAIdx++)
    {
        BugLoop *loopA = world->loops+loopAIdx;
        for(int loopBIdx = loopAIdx+1;
                loopBIdx < world->nLoops;
                loopBIdx++)
        {
            BugLoop *loopB = world->loops+loopBIdx;
            CollideLoops(world, loopA, loopB);
        }
    }
#endif
} 


