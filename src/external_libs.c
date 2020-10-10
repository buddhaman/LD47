#include <SDL2/SDL.h>
#include "GL/gl3w.cpp"

#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_FIXED_TYPES
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear_sdl_gl3.h"

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

