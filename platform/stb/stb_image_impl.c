#if defined(CUPCAKE_GNW) || defined(CUPCAKE_ABI)
#include "gw_malloc.h"

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_GIF
#define STBI_ASSERT(x) ((void)0)
#define STBI_MALLOC(sz) ram_malloc(sz)
#define STBI_REALLOC(p, sz) realloc(p, sz)
#define STBI_FREE(p) free(p)
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"
