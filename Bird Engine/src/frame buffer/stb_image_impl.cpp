#include <crock.h>

#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_THREAD_LOCALS
#define STBI_NO_FAILURE_STRINGS
#define STBI_ASSERT(x) ((void)0)

#define STBI_MALLOC(sz)          memoria_malloc(sz)
#define STBI_REALLOC(p, newsz)   memoria_realloc(p, newsz)
#define STBI_FREE(p)             memoria_free(p)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
