
static int SEED = 0;

static int hash[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40, 185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204, 9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81, 70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13, 203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41, 164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105, 228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89, 232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174, 193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145, 101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167, 135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255, 114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219};

int noise2(int x, int y)
{
    int tmp = hash[(y + SEED) % 256];
    return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int+1, y_int);
    int u = noise2(x_int, y_int+1);
    int v = noise2(x_int+1, y_int+1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x*freq;
    float ya = y*freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for(i=0; i<depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}

internal inline Vec3
ARGBToVec3(ui32 hex)
{
    return vec3( (hex >> 16 & 255)/255.0,
            (hex >> 8 & 255)/255.0,
            (hex & 255)/255.0);
}

internal void
InitCamera(Camera *camera)
{
    camera->spherical = vec3(3*M_PI/2.0, 1, 30);
}

internal void
UpdateCamera(Camera *camera, r32 width, r32 height)
{
    r32 theta = camera->spherical.x;
    r32 phi = camera->spherical.y;
    r32 r = camera->spherical.z;
    camera->pos = vec3(
            cosf(theta) * cosf(phi) * r + camera->lookAt.x,
            sinf(theta) * cosf(phi) * r + camera->lookAt.y,
            sinf(phi) * r + camera->lookAt.z
            );
    Mat4 transform = m4_look_at(camera->pos, camera->lookAt, vec3(0,0,1));
    camera->transform = m4_mul(
            m4_perspective(90, width/height, 1, 1000),
            transform
            );
}

internal void
InitMesh(MemoryArena *arena, Mesh *mesh, int maxVertices)
{
    mesh->colorState = vec3(1,1,1);
    mesh->nVertices = 0;
    mesh->maxVertices = maxVertices;
    mesh->vertices = PushArray(arena, Vec3, maxVertices);
    mesh->colors = PushArray(arena, Vec3, maxVertices);
    mesh->normals = PushArray(arena, Vec3, maxVertices);
    mesh->nIndices = 0;
    mesh->maxIndices = maxVertices;
    mesh->indices = PushArray(arena, ui32, maxVertices);
}

internal Mesh *
CreateMesh(MemoryArena *arena, int maxVertices)
{
    Mesh *mesh = PushStruct(arena, Mesh);
    InitMesh(arena, mesh, maxVertices);
    return mesh;
}

internal void
ClearMesh(Mesh *mesh)
{
    mesh->nVertices = 0;
    mesh->nIndices = 0;
}

internal void
InitModel(MemoryArena *arena, Model *model, int maxVertices)
{
    glGenVertexArrays(1, &model->vao);
    glGenBuffers(1, &model->vbo);
    glGenBuffers(1, &model->ebo);
    model->stride = 9;
    model->vertexBufferSize = 0;
    model->maxVertexBufferSize = maxVertices*model->stride;
    model->maxIndexBufferSize = maxVertices;
    model->indexBufferSize = 0;

    model->vertexBuffer = PushArray(arena, r32, maxVertices*model->stride);
    model->indexBuffer = PushArray(arena, ui32, maxVertices);

    glBindVertexArray(model->vao);
    glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
    glEnableVertexAttribArray(0);       // Positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
            model->stride*sizeof(r32), (void *)0);
    glEnableVertexAttribArray(1);       // Colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 
            model->stride*sizeof(r32), (void *)(3*sizeof(r32)));
    glEnableVertexAttribArray(2);       // Normals
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
            model->stride*sizeof(r32), (void *)(6*sizeof(r32)));
}

internal void
SetModelFromMesh(Model *model, Mesh *mesh, GLenum drawMode)
{
    model->vertexBufferSize = 0;
    model->indexBufferSize = 0;
    for(int vertexIdx = 0;
            vertexIdx < mesh->nIndices;
            vertexIdx++)
    {
        Vec3 vert = mesh->vertices[vertexIdx];
        Vec3 col = mesh->colors[vertexIdx];
        Vec3 norm = mesh->normals[vertexIdx];
        model->vertexBuffer[model->vertexBufferSize++] = vert.x;
        model->vertexBuffer[model->vertexBufferSize++] = vert.y;
        model->vertexBuffer[model->vertexBufferSize++] = vert.z;
        model->vertexBuffer[model->vertexBufferSize++] = col.x;
        model->vertexBuffer[model->vertexBufferSize++] = col.y;
        model->vertexBuffer[model->vertexBufferSize++] = col.z;
        model->vertexBuffer[model->vertexBufferSize++] = norm.x;
        model->vertexBuffer[model->vertexBufferSize++] = norm.y;
        model->vertexBuffer[model->vertexBufferSize++] = norm.z;
        Assert(model->vertexBufferSize < model->maxVertexBufferSize);
    }
    Assert(model->maxIndexBufferSize > mesh->nIndices);
    memcpy(model->indexBuffer, mesh->indices, mesh->nIndices*sizeof(ui32));
    glBindVertexArray(model->vao);
    model->indexBufferSize = mesh->nIndices;

    glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
    glBufferData(GL_ARRAY_BUFFER, model->vertexBufferSize*sizeof(r32), 
            model->vertexBuffer, drawMode);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->indexBufferSize*sizeof(ui32), 
            model->indexBuffer, drawMode);
}

internal void
PushVertex(Mesh *mesh, Vec3 pos, Vec3 normal)
{
    mesh->vertices[mesh->nVertices] = pos;
    mesh->colors[mesh->nVertices] = mesh->colorState;
    mesh->normals[mesh->nVertices] = normal;
    mesh->nVertices++;
    Assert(mesh->nVertices < mesh->maxIndices);
}

internal void 
PushIndex(Mesh *mesh, ui32 index)
{
    mesh->indices[mesh->nIndices++] = index;
    Assert(mesh->nIndices < mesh->maxVertices);
}

internal void
PushTriangle(Mesh *mesh, Vec3 p0, Vec3 p1, Vec3 p2)
{
    Vec3 diff0 = v3_sub(p1, p0);
    Vec3 diff1 = v3_sub(p2, p0);
    Vec3 normal = v3_norm(v3_cross(diff0, diff1));
    ui32 nVertices = mesh->nVertices;
    PushVertex(mesh, p0, normal);
    PushVertex(mesh, p1, normal);
    PushVertex(mesh, p2, normal);
    PushIndex(mesh, nVertices);
    PushIndex(mesh, nVertices+1);
    PushIndex(mesh, nVertices+2);
}

// Assume in same plane
internal void
PushQuad(Mesh *mesh, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3)
{
    Vec3 diff0 = v3_sub(p1, p0);
    Vec3 diff1 = v3_sub(p2, p0);
    Vec3 normal = v3_norm(v3_cross(diff0, diff1));
    ui32 nVertices = mesh->nVertices;
    PushVertex(mesh, p0, normal);
    PushVertex(mesh, p1, normal);
    PushVertex(mesh, p2, normal);
    PushVertex(mesh, p3, normal);
    PushIndex(mesh, nVertices);
    PushIndex(mesh, nVertices+1);
    PushIndex(mesh, nVertices+2);
    PushIndex(mesh, nVertices+2);
    PushIndex(mesh, nVertices+3);
    PushIndex(mesh, nVertices);
}

internal inline void
PushTrapezoid(Mesh *mesh, Vec3 from, Vec3 to, r32 fromWidth, r32 toWidth, Vec3 normal)
{
    Vec3 diff = v3_sub(to, from);
    Vec3 perp = v3_cross(diff, normal);
    perp = v3_norm(perp);
    Vec3 perp0 = v3_muls(perp, fromWidth/2);
    Vec3 perp1 = v3_muls(perp, toWidth/2);
    ui32 nVertices = mesh->nVertices;
    PushVertex(mesh, v3_add(from, v3_muls(perp0,-1)), normal);
    PushVertex(mesh, v3_add(from, perp0), normal);
    PushVertex(mesh, v3_add(to, perp1), normal);
    PushVertex(mesh, v3_add(to, v3_muls(perp1, -1)), normal);
    PushIndex(mesh, nVertices);
    PushIndex(mesh, nVertices+1);
    PushIndex(mesh, nVertices+2);
    PushIndex(mesh, nVertices+2);
    PushIndex(mesh, nVertices+3);
    PushIndex(mesh, nVertices);
}

internal inline void 
PushLine(Mesh *mesh, Vec3 from, Vec3 to,  r32 width, Vec3 normal)
{
    PushTrapezoid(mesh, from, to, width, width, normal);
}

internal inline void
PushLineCircle(Mesh *mesh, Vec3 center, r32 radius, int nPoints, r32 lineWidth)
{
    Vec3 prev = vec3(center.x+radius, center.y, center.z);
    for(int point = 0;
            point < nPoints;
            point++)
    {
        r32 angle = point * (M_PI*2.0 / (nPoints-1));
        r32 c = cosf(angle);
        r32 s = sinf(angle);
        Vec3 p = v3_add(center, vec3(c*radius, s*radius, 0));
        PushLine(mesh, prev, p, lineWidth, vec3(0,0,1));
        prev = p;
    }
}

internal r32
RandomFloat(r32 min, r32 max)
{
    return min + (max-min)*(rand()%10000)/10000.0;
}

internal inline Vec3
lerp(Vec3 from, Vec3 to, r32 lambda)
{
    return v3_add(v3_muls(from, 1.0-lambda), v3_muls(to, lambda));
}

internal void 
PushFlatField(Mesh *mesh, r32 width, r32 height)
{
    PushQuad(mesh, vec3(0,0,0), vec3(width, 0, 0),
            vec3(width, height, 0), vec3(0, height, 0));
}

internal void
PushHeightField(Mesh *mesh, r32 tileSize, int width, int height)
{
    r32 xScale = 1.0;
    r32 yScale = 1.0;
    Vec3 positions[(width)*(height)];
    mesh->colorState = ARGBToVec3(0xfffffb87);
    r32 depth = 4;
    for(int y = 0; y < height; y++)
    for(int x = 0; x < width; x++)
    {
        r32 perlin = perlin2d(x*xScale, y*yScale, 1, 7);
        r32 z = depth*perlin - depth;
        r32 dev = 0.2;
        positions[x+y*width] = vec3(tileSize*(x+RandomFloat(-dev, dev)),
                    tileSize*(y+RandomFloat(-dev, dev)), 
                    z);
    }
    for(int y = 0; y < height-1; y++)
    for(int x = 0; x < width-1; x++)
    {
        Vec3 p0 = positions[x+y*width];
        Vec3 p1 = positions[x+1+y*width];
        Vec3 p2 = positions[x+1+(y+1)*width];
        Vec3 p3 = positions[x+(y+1)*width];
        PushTriangle(mesh, p0, p1, p2);
        PushTriangle(mesh, p2, p3, p0);
    }
}

internal inline void
PushCube(Mesh *mesh, Vec3 origin, Vec3 dimensions)
{
    r32 x = dimensions.x;
    r32 y = dimensions.y;
    r32 z = dimensions.z;
    Vec3 p000 = origin;
    Vec3 p001 = v3_add(origin, vec3(0, 0, z));
    Vec3 p010 = v3_add(origin, vec3(0, y, 0));
    Vec3 p011 = v3_add(origin, vec3(0, y, z));
    Vec3 p100 = v3_add(origin, vec3(x, 0, 0));
    Vec3 p101 = v3_add(origin, vec3(x, 0, z));
    Vec3 p110 = v3_add(origin, vec3(x, y, 0));
    Vec3 p111 = v3_add(origin, vec3(x, y, z));
    PushQuad(mesh,
            p010,
            p110,
            p100,
            p000);
    PushQuad(mesh,
            p001,
            p011,
            p010,
            p000);
    PushQuad(mesh,
            p001,
            p101,
            p111,
            p011);
    PushQuad(mesh,
            p100,
            p110,
            p111,
            p101);
    PushQuad(mesh,
            p000,
            p100,
            p101,
            p001);
    PushQuad(mesh,
            p011,
            p111,
            p110,
            p010);
}

internal inline void
PushCactus(Mesh *mesh, Vec3 pos, r32 scale)
{
    mesh->colorState = vec3(0,1,0);
    Vec3 dims = vec3(scale, scale, scale*5);
    r32 leftArmZ = RandomFloat(0.5, 0.8)*dims.z;
    r32 rightArmZ = RandomFloat(0.5, 0.8)*dims.z;
    r32 leftArmWidth = RandomFloat(scale*2, scale*3);
    r32 rightArmWidth = RandomFloat(scale*2, scale*3);
    r32 armDepthMin = scale*2;
    r32 armDepthMax = scale*3;
    r32 armScale = 2*scale/3;
    PushCube(mesh, vec3(pos.x-leftArmWidth, pos.y-armScale/2, leftArmZ-armScale/2),
            vec3(leftArmWidth, armScale, armScale));
    PushCube(mesh, vec3(pos.x-leftArmWidth, pos.y-armScale/2, leftArmZ-armScale/2),
            vec3(armScale, armScale,  RandomFloat(armDepthMin, armDepthMax)));

    PushCube(mesh, vec3(pos.x, pos.y-armScale/2, rightArmZ-armScale/2),
            vec3(rightArmWidth, armScale, armScale));
    PushCube(mesh, vec3(pos.x+rightArmWidth, pos.y-armScale/2, rightArmZ-armScale/2),
            vec3(armScale, armScale,  RandomFloat(armDepthMin, armDepthMax)));

    PushCube(mesh, vec3(pos.x-dims.x/2, pos.y-dims.y/2, pos.z), dims);
}

internal void
RenderModel(Model *model)
{
    glBindVertexArray(model->vao);
    glDrawElements(GL_TRIANGLES, model->indexBufferSize, GL_UNSIGNED_INT, 0);
}

