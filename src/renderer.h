typedef struct 
{
    Vec3 colorState;
    int nVertices;
    int maxVertices;
    Vec3 *vertices;
    Vec3 *colors;
    Vec3 *normals;
    int nIndices;
    int maxIndices;
    ui32 *indices;
} Mesh;

typedef struct 
{
    ui32 vao;
    ui32 vbo;
    ui32 ebo;
    int stride;

    int vertexBufferSize;
    int maxVertexBufferSize;
    r32 *vertexBuffer;

    int indexBufferSize;
    int maxIndexBufferSize;
    ui32 *indexBuffer;
} Model;


typedef struct
{
    Vec3 pos;
    Vec3 lookAt;
    Vec3 spherical;
    Mat4 transform;
} Camera;


