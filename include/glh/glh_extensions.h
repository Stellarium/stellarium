// Comments:
//
//   The trick with GLH_EXT_SINGLE_FILE is that you need to have it defined in
// exactly one cpp file because it piggy-backs the function implementations in
// the header.  You don't want multiple implementations defined or the linker
// gets mad, but one file must have the implementations in it or the linker
// gets mad for different reasons.
// 
//   The goal was to avoid having this helper require a separate cpp file.  One
// thing you could do is just have a glh_extensions.cpp that did
// 
// #define GLH_EXT_SINGLE_FILE
// #include <glh_extensions.h>
//
// and make it the only file that ever defines GLH_EXT_SINGLE_FILE.

#ifndef GLH_EXTENSIONS
#define GLH_EXTENSIONS

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32)
# include <windows.h>
#endif

#ifdef MACOS
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef WIN32
# include <GL/wglext.h>
#endif

#define CHECK_MEMORY(ptr) \
    if (NULL == ptr) { \
        printf("Error allocating memory in file %s, line %d\n", __FILE__, __LINE__); \
        exit(-1); \
    }

#ifdef GLH_EXT_SINGLE_FILE
#  define GLH_EXTENSIONS_SINGLE_FILE  // have to do this because glh_genext.h unsets GLH_EXT_SINGLE_FILE
#endif

#if (defined(WIN32) || defined(UNIX))
#include "glh_genext.h"
#elif defined(MACOS)
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GLH_EXTENSIONS_SINGLE_FILE
static char *unsupportedExts = NULL;
static char *sysExts = NULL;
#ifndef GL_SHADER_CONSISTENT_NV
#define GL_SHADER_CONSISTENT_NV           0x86DD
#endif
#ifndef GL_TEXTURE_SHADER_NV
#define GL_TEXTURE_SHADER_NV              0x86DE
#endif
#ifndef GL_SHADER_OPERATION_NV
#define GL_SHADER_OPERATION_NV            0x86DF
#endif

static int ExtensionExists(const char* extName, const char* sysExts)
{
    char *padExtName = (char*)malloc(strlen(extName) + 2);
    strcat(strcpy(padExtName, extName), " ");

    if (0 == strcmp(extName, "GL_VERSION_1_2")) {
        const char *version = (const char*)glGetString(GL_VERSION);
        if (strstr(version, "1.0") == version || strstr(version, "1.1") == version) {
            return GL_FALSE;
        } else {
            return GL_TRUE;
        }
    }
    if (0 == strcmp(extName, "GL_VERSION_1_3")) {
        const char *version = (const char*)glGetString(GL_VERSION);
        if (strstr(version, "1.0") == version ||
            strstr(version, "1.1") == version ||
            strstr(version, "1.2") == version) {
            return GL_FALSE;
        } else {
            return GL_TRUE;
        }
    }
    if (0 == strcmp(extName, "GL_VERSION_1_4")) {
        const char *version = (const char*)glGetString(GL_VERSION);
        if (strstr(version, "1.0") == version ||
            strstr(version, "1.1") == version ||
            strstr(version, "1.2") == version || 
            strstr(version, "1.3") == version) {
            return GL_FALSE;
        } else {
            return GL_TRUE;
        }
    }
    if (0 == strcmp(extName, "GL_VERSION_1_5")) {
        const char *version = (const char*)glGetString(GL_VERSION);
        if (strstr(version, "1.0") == version ||
            strstr(version, "1.1") == version ||
            strstr(version, "1.2") == version || 
            strstr(version, "1.3") == version || 
            strstr(version, "1.4") == version) {
            return GL_FALSE;
        } else {
            return GL_TRUE;
        }
    }
    if (strstr(sysExts, padExtName)) {
        free(padExtName);
        return GL_TRUE;
    } else {
        free(padExtName);
        return GL_FALSE;
    }
}

static const char* EatWhiteSpace(const char *str)
{
    for (; *str && (' ' == *str || '\t' == *str || '\n' == *str); str++);
    return str;
}

static const char* EatNonWhiteSpace(const char *str)
{
    for (; *str && (' ' != *str && '\t' != *str && '\n' != *str); str++);
    return str;
}

int glh_init_extensions(const char *origReqExts)
{
    // Length of requested extensions string
    size_t reqExtsLen;
    char *reqExts;
    // Ptr for individual extensions within reqExts
    char *reqExt;
    int success = GL_TRUE;
    // build space-padded extension string
    if (NULL == sysExts) {
        const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
        size_t sysExtsLen = strlen(extensions);
        const char *winsys_extensions = "";     
        size_t winsysExtsLen = 0;
#if defined(WIN32)
        {
            PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
            wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
            if(wglGetExtensionsStringARB)
            {
                winsys_extensions = wglGetExtensionsStringARB(wglGetCurrentDC());
                winsysExtsLen = strlen(winsys_extensions);
            }
        }
#elif defined(UNIX)
        {

          winsys_extensions = glXQueryExtensionsString (glXGetCurrentDisplay(),DefaultScreen(glXGetCurrentDisplay())) ;
          winsysExtsLen = strlen (winsys_extensions);
        }
#endif
        // Add 2 bytes, one for padding space, one for terminating NULL
        sysExts = (char*)malloc(sysExtsLen + winsysExtsLen + 3);
        CHECK_MEMORY(sysExts);
        strcpy(sysExts, extensions);
        sysExts[sysExtsLen] = ' ';
        sysExts[sysExtsLen + 1] = 0;
        strcat(sysExts, winsys_extensions);
        sysExts[sysExtsLen + 1 + winsysExtsLen] = ' ';
        sysExts[sysExtsLen + 1 + winsysExtsLen + 1] = 0;
    }

    if (NULL == origReqExts)
        return GL_TRUE;
    reqExts = strdup(origReqExts);
    reqExtsLen = strlen(reqExts);
    if (NULL == unsupportedExts) {
        unsupportedExts = (char*)malloc(reqExtsLen + 2);
    } else if (reqExtsLen > strlen(unsupportedExts)) {
        unsupportedExts = (char*)realloc(unsupportedExts, reqExtsLen + 2);
    }
    CHECK_MEMORY(unsupportedExts);
    *unsupportedExts = 0;

    // Parse requested extension list
    for (reqExt = reqExts;
        (reqExt = (char*)EatWhiteSpace(reqExt)) && *reqExt;
        reqExt = (char*)EatNonWhiteSpace(reqExt))
    {
        char *extEnd = (char*)EatNonWhiteSpace(reqExt);
        char saveChar = *extEnd;
        *extEnd = (char)0;

#if (defined(WIN32) || defined(UNIX))
        if (!ExtensionExists(reqExt, sysExts) || !glh_init_extension(reqExt)) {
#elif defined(MACOS)
        if (!ExtensionExists(reqExt, sysExts)) {    // don't try to get function pointers if on MacOS
#endif
            // add reqExt to end of unsupportedExts
            strcat(unsupportedExts, reqExt);
            strcat(unsupportedExts, " ");
            success = GL_FALSE;
        }
        *extEnd = saveChar;
    }
    free(reqExts);
    return success;
}

const char* glh_get_unsupported_extensions()
{
    return (const char*)unsupportedExts;
}

void glh_shutdown_extensions()
{
    if (unsupportedExts)
    {
        free(unsupportedExts);
        unsupportedExts = NULL;
    }
    if (sysExts)
    {
        free(sysExts);
        sysExts = NULL;
    }
}

int glh_extension_supported(const char *extension)
{
    static const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;
    
    // Extension names should not have spaces.
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0')
      return 0;
    
    if (!extensions)
      extensions = glGetString(GL_EXTENSIONS);

    // It takes a bit of care to be fool-proof about parsing the
    // OpenGL extensions string.  Don't be fooled by sub-strings,
    // etc.
    start = extensions;
    for (;;) 
    {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ') 
        {
            if (*terminator == ' ' || *terminator == '\0') 
            {
                return 1;
            }
        }
        start = terminator;
    }
    return 0;
}

#else
int glh_init_extensions(const char *origReqExts);
const char* glh_get_unsupported_extensions();
void glh_shutdown_extensions();
int glh_extension_supported(const char *extension);
#endif /* GLH_EXT_SINGLE_FILE */

#ifdef __cplusplus
}
#endif

#endif /* GLH_EXTENSIONS */
