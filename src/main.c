#include "external_headers.h"

#define DebugOut(args...) printf(args); printf("\t\t %s:%d\n", __FILE__, __LINE__)
#define Assert(expr) if(!(expr)) {DebugOut("assert failed "#expr""); \
    *((int *)0)=0;}

#define local_persist static
#define internal static
#define global_variable static

typedef unsigned char ui8;
typedef unsigned short ui16;
typedef unsigned int ui32;
typedef unsigned long ui64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;

typedef float r32;
typedef double r64;

typedef i32 b32;

#define MAX_VERTEX_MEMORY 512*1024
#define MAX_ELEMENT_MEMORY 128*1024

// Own files
#include "cool_memory.h"
#include "tims_math.h"
#include "app_state.h"
#include "renderer.h"
#include "bug.h"

#include "cool_memory.c"
#include "tims_math.c"
#include "app_state.c"
#include "renderer.c"
#include "bug.c"

#define FRAMES_PER_SECOND 60

const char *
ReadEntireFile(char *path)
{
    char *buffer = NULL;
    size_t stringSize, readSize;
    FILE *handler = fopen(path, "r");
    if(handler)
    {
        fseek(handler, 0, SEEK_END);
        stringSize = ftell(handler);
        rewind(handler);
        buffer = (char *)malloc(sizeof(char) * (stringSize + 1));
        readSize = fread(buffer, sizeof(char), stringSize, handler);
        buffer[stringSize] = 0;

        if(stringSize!=readSize)
        {
            free(buffer);
            buffer = NULL;
        }
        fclose(handler);
        return buffer;
    }
    else
    {
        DebugOut("Can't read file %s", path);
        return NULL;
    }
}

ui32
CreateAndCompileShaderSource(const char * const*source, GLenum shaderType)
{
    ui32 shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, source, NULL);
    glCompileShader(shader);
    i32 succes;
    char infolog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succes);
    if(!succes)
    {
        glGetShaderInfoLog(shader, 512, NULL, infolog);
        DebugOut("%s", infolog);
    }
    else
    {
        DebugOut("Shader %u compiled succesfully.", shader);
    }
    return shader;
}

typedef enum
{
    STATE_MENU,
    STATE_GAME
} GameState;

ui32 
CreateAndLinkShaderProgram(ui32 vertexShader, ui32 fragmentShader)
{
    ui32 shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    i32 succes;
    char infolog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &succes);
    if(!succes)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infolog);
        DebugOut("%s", infolog);
    }
    else
    {
        DebugOut("Shader program %u linked succesfully.", shaderProgram);
    }
    return shaderProgram;
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    ma_decoder *pDecoder = (ma_decoder *)pDevice->pUserData;
    if(pDecoder==NULL)
    {
        return;
    }
    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);

    (void)pInput;
}

void
SetupWorld(MemoryArena *arena, World *world, r32 aiSpeed)
{
    // Creating the world
    world->width = 500;
    world->height = 320;
    world->nBugs = 0;
    world->maxBugs = 1200;
    world->bugs = PushArray(arena, Bug, world->maxBugs);
    world->maxLoops = 32;
    world->aiSpeed = aiSpeed;
    world->loops = PushArray(arena, BugLoop, world->maxLoops);
    world->loopBugPointers = PushArray(arena, Bug*, world->maxBugs);
    world->loopColors[0] = ARGBToVec3(0xffff0000);
    world->loopColors[1] = ARGBToVec3(0xff006400);
    world->loopColors[2] = ARGBToVec3(0xff191970);
    world->loopColors[3] = ARGBToVec3(0xff2f4f4f);
    world->loopColors[4] = ARGBToVec3(0xff00ff00);
    world->loopColors[5] = ARGBToVec3(0xff00ffff);
    world->loopColors[6] = ARGBToVec3(0xffff00ff);
    world->loopColors[7] = ARGBToVec3(0xffffb6c1);
    world->isLoopDistributionDirty = 1;
    int nLoops = 32;
    for(int loopN = 0;
            loopN < nLoops;
            loopN++)
    {
        AddLoop(arena, world);
        int nBugsInLoop = loopN == 0 ? 50 : 30;
        for(int bugIdx = 0;
                bugIdx < nBugsInLoop;
                bugIdx++)
        {
            Bug *bug = AddBug(world, loopN);
        }
    }
}

void 
SetupWorldMesh(World *world, Mesh *mesh)
{
    ClearMesh(mesh);
    r32 tileSize = 10;
    int xTiles = (int)(world->width/tileSize);
    int yTiles = (int)(world->height/tileSize);
    mesh->colorState = ARGBToVec3(0xff5cf508);
    PushHeightField(mesh, tileSize, xTiles+1, yTiles+1);
    int nCactus = 5;
    for(int i = 0; i < nCactus; i++)
    {
        Vec3 randPos = vec3(RandomFloat(0, world->width), RandomFloat(0, world->height), -1);
        PushCactus(mesh, randPos, 5);
    }
}

void 
ResetWorld(MemoryArena *arena, World **world, Mesh *groundMesh, Model *groundModel, r32 aiSpeed)
{
    ClearArena(arena);
    *world = PushStruct(arena, World);
    SetupWorld(arena, *world, aiSpeed);
    ClearMesh(groundMesh);
    SetupWorldMesh(*world, groundMesh);
    SetModelFromMesh(groundModel, groundMesh, GL_STATIC_DRAW);
}

int 
main(int argc, char**argv)
{

#if 0
    // Audio setup 
    ma_result result;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;
    result = ma_decoder_init_file("file_example_WAV_1MG.wav", NULL, &decoder);
    if(result != MA_SUCCESS)
    {
        DebugOut("Could not load audio");
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &decoder;

    if(ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        DebugOut("Failed to open playback device.");
    }

    if(ma_device_start(&device) != MA_SUCCESS)
    {
        DebugOut("Failed to start playback device.");
    }
#endif

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)!=0)
    {
        DebugOut("Does not work\n");
    }

    srand(time(0));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = 
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    i32 screen_width = 1280;
    i32 screen_height = 720;

    SDL_Window *window = SDL_CreateWindow("Cool", SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, screen_width, screen_height, window_flags);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    SDL_GL_SetSwapInterval(1);
    
    b32 err = gl3wInit() != 0;
    if(err)
    {
        DebugOut("Failed to initialize OpenGl Loader!\n");
    }

    // Setup shaders
    const char *const solidFragSource = ReadEntireFile("shaders/solid.frag");
    const char *const solidVertSource = ReadEntireFile("shaders/solid.vert");
    
    ui32 fragmentShader = CreateAndCompileShaderSource(&solidFragSource, GL_FRAGMENT_SHADER);
    ui32 vertexShader = CreateAndCompileShaderSource(&solidVertSource, GL_VERTEX_SHADER);
    ui32 simpleShader = CreateAndLinkShaderProgram(fragmentShader, vertexShader);

    ui32 transformLocation = glGetUniformLocation(simpleShader, "transform");
    ui32 lightDirLocation = glGetUniformLocation(simpleShader, "lightDir");

    r32 tSize = 0.5;
    r32 vertices[] = {-tSize, -tSize, 0.0,
    tSize, -tSize, 0.0,
    tSize, tSize, 0.0};

    ui32 vao;
    glGenVertexArrays(1, &vao);
    ui32 vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(r32), (void *)0);


    // Setup nuklear
    struct nk_context *ctx;
    ctx = nk_sdl_init(window);
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    // Creating appstate
    AppState *appState = (AppState *)malloc(sizeof(AppState));
    *appState = (AppState){};

    r32 time = 0.0;
    r32 deltaTime = 0.0;
    r32 timeSinceLastFrame = 0.0;
    r32 updateTime = 1.0/FRAMES_PER_SECOND;
    ui32 frameStart = SDL_GetTicks(); 
    // Timing
    b32 done = 0;
    ui32 frameCounter = 0;
    r32 aiSpeed = 1.0;
    
    // Camera
    Camera camera;
    InitCamera(&camera);
    camera.lookAt = vec3(10,10,0);

    MemoryArena *gameArena = CreateMemoryArena(1024*1024*20);
    MemoryArena *renderArena = CreateMemoryArena(1024*1024*20);

    World *world = PushStruct(gameArena, World);
    SetupWorld(gameArena, world, aiSpeed);

    Model *groundModel = PushStruct(renderArena, Model);
    Mesh *groundMesh = CreateMesh(renderArena, 20000);
    InitModel(gameArena, groundModel, 20000);

    SetupWorldMesh(world, groundMesh);
    SetModelFromMesh(groundModel, groundMesh, GL_STATIC_DRAW);

    Mesh *dynamicMesh = CreateMesh(renderArena, 100000);
    Model *dynamicModel = PushStruct(renderArena, Model);
    InitModel(gameArena, dynamicModel, 100000);

    DebugOut("%d bugs", world->nBugs);
    BugLoop *playerLoop = world->loops;

    DebugOut("game arena : %lu / %lu bytes used. %lu procent", 
            gameArena->used, gameArena->size, (gameArena->used*100)/gameArena->size);
    DebugOut("render arena : %lu / %lu bytes used. %lu procent", 
            renderArena->used, renderArena->size, (renderArena->used*100)/renderArena->size);

    GameState state = STATE_MENU;
    playerLoop->pos.x=world->width/2;
    playerLoop->pos.y=world->height/2;

    while(!done)
    {
        SDL_Event event;
        nk_input_begin(ctx);
        ResetKeyActions(appState);
        while(SDL_PollEvent(&event))
        {
            nk_sdl_handle_event(&event);
            switch(event.type)
            {

            case SDL_KEYUP:
            {
                RegisterKeyAction(appState, MapKeyCodeToAction(event.key.keysym.sym), 0);
            } break;
            case SDL_KEYDOWN:
            {
                RegisterKeyAction(appState, MapKeyCodeToAction(event.key.keysym.sym), 1);
            } break;

            case SDL_QUIT:
            {
                done = 1;
            } break;

            default:
            {
                if(event.type == SDL_WINDOWEVENT
                        && event.window.event == SDL_WINDOWEVENT_CLOSE
                        && event.window.windowID == SDL_GetWindowID(window))
                {
                    done = 1;
                }
            } break;

            }
        }
        nk_input_end(ctx);
        // Set Appstate
        SDL_GetWindowSize(window, &appState->screenWidth, &appState->screenHeight);
        appState->ratio = (r32)appState->screenHeight / ((r32)appState->screenWidth);

        // Clear screen
        Vec3 clearColor = ARGBToVec3(0xffe0fffe);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, appState->screenWidth, appState->screenHeight);

        // My rendering
        Vec3 lightDir = v3_norm(vec3(-1,1,-1));
        
        // Update Camera
        glUseProgram(simpleShader);
        r32 camSpeed = 2;
        r32 zoomSpeed = 0.98;
        if(state==STATE_GAME)
        {
            if(IsKeyActionDown(appState, ACTION_Z))
            {
                camera.spherical.z*=zoomSpeed;
            }
            if(IsKeyActionDown(appState, ACTION_X))
            {
                camera.spherical.z/=zoomSpeed;
            }
            if(IsKeyActionDown(appState, ACTION_UP))
            {
                playerLoop->pos.y+=camSpeed;
            }
            if(IsKeyActionDown(appState, ACTION_DOWN))
            {
                playerLoop->pos.y-=camSpeed;
            }
            if(IsKeyActionDown(appState, ACTION_LEFT))
            {
                playerLoop->pos.x-=camSpeed;
            }
            if(IsKeyActionDown(appState, ACTION_RIGHT))
            {
                playerLoop->pos.x+=camSpeed;
            }
            if(IsKeyActionDown(appState, ACTION_Q))
            {
                camera.spherical.y-=0.1;
                if(camera.spherical.y < 0.1) camera.spherical.y = 0.1;
            }
            if(IsKeyActionDown(appState, ACTION_E))
            {
                camera.spherical.y+=0.1;
                if(camera.spherical.y > M_PI/2-0.1) camera.spherical.y = M_PI/2-0.1;
            }
            if(IsKeyActionDown(appState, ACTION_R))
            {
            }
        }
        else
        {
            playerLoop->pos.x+=cosf(time)*(1.2+sinf(2*time));
            playerLoop->pos.y+=sinf(time)*(1.2+cosf(time));
            camera.spherical.z = 80+sinf(time)*20;
        }
        // Update loop pos
        camera.lookAt = playerLoop->pos;
        UpdateCamera(&camera, appState->screenWidth, appState->screenHeight);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Set uniforms
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, (GLfloat*)&camera.transform);
        glUniform3fv(lightDirLocation, 1, (GLfloat*)&lightDir);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        RenderModel(groundModel);

        UpdateAndRenderLoops(world, dynamicMesh);
        UpdateAndRenderBugs(world, dynamicMesh);

        // Render dynamic model
        SetModelFromMesh(dynamicModel, dynamicMesh, GL_DYNAMIC_DRAW);
        glDisable(GL_CULL_FACE);
        RenderModel(dynamicModel);
        ClearMesh(dynamicMesh);

        // Menu
        r32 menuWidth = 330;
        r32 menuHeight = 250;
        if(state==STATE_MENU)
        {
            nk_begin(ctx, "Mehnu", 
                    nk_rect(appState->screenWidth/2-menuWidth/2, appState->screenHeight/2-menuHeight/2, 
                        menuWidth, menuHeight),
                        NK_WINDOW_BORDER | NK_WINDOW_TITLE);
            nk_layout_row_static(ctx, 30, 300, 1);
            if(nk_button_label(ctx, "begni bgame"))
            {
                state=STATE_GAME;;
                ResetWorld(gameArena, &world, groundMesh, groundModel, aiSpeed);
            }
            nk_label_wrap(ctx, "Insrtuctions: Cllect al bugs in u loop");
            nk_label_wrap(ctx, "MOve: WASD/arrows, zoom: Z, X, Tilst camera: Q, E");
            nk_label_wrap(ctx, "Diffculty");
            nk_slider_float(ctx, 0.8, &aiSpeed, 4, 0.2);
                    
            nk_end(ctx);
        }
        else
        {
            nk_begin(ctx, "game", nk_rect(0,0,400,40), 0);
            nk_layout_row_static(ctx, 30, 250, 1);
            nk_labelf_wrap(ctx, "numnbr of bfugs %d", playerLoop->nBugs);
            nk_end(ctx);
            // If won
            if(playerLoop->nBugs <= 0 || playerLoop->nBugs >= world->nBugs*0.8)
            {
                nk_begin(ctx, playerLoop->nBugs <= 0 ? "Youu lost!" : "u won",
                        nk_rect(appState->screenWidth/2-menuWidth/2, 0, menuWidth, 80),
                            NK_WINDOW_BORDER | NK_WINDOW_TITLE);
                nk_layout_row_static(ctx, 30, 250, 1);
                if(nk_button_label(ctx, "K"))
                {
                    state=STATE_MENU;;
                }
                nk_end(ctx);
            }
        }

        nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);

        // frame timing
        ui32 frameEnd = SDL_GetTicks();
#if 0
        if(frameEnd > (lastSecond+1)*1000)
        {
            // new second
            lastSecond = frameEnd/1000;
            DebugOut("Frames per second = %d\n", frameCounter);
            frameCounter = 0;
        }
#endif
        ui32 frameTicks = frameEnd-frameStart;
        frameStart=frameEnd;
        deltaTime=((r32)frameTicks)/1000.0;
        time+=deltaTime;
        timeSinceLastFrame+=deltaTime;
        if(timeSinceLastFrame < updateTime)
        {
            i32 waitForTicks = (i32)((updateTime-timeSinceLastFrame)*1000);
            if(waitForTicks > 0)
            {
                //DebugOut("this happened, waitForTicks = %d\n", waitForTicks);
                if(waitForTicks < 2*updateTime*1000)
                {
                    SDL_Delay(waitForTicks);
                    timeSinceLastFrame-=waitForTicks/1000.0;
                }
                else
                {
                    timeSinceLastFrame = 0;
                }
            }
        }
        //DebugOut("time = %f, deltaTime = %f, ticks = %d\n", time, deltaTime, frameTicks);
        SDL_GL_SwapWindow(window);
        frameCounter++;
    }
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

