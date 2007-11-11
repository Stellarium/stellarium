/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* StormMesa implementation of SDL OpenGL support */

#include "SDL_cgxgl_c.h"
#include "SDL_cgxvideo.h"

#if SDL_VIDEO_OPENGL
AmigaMesaContext glcont=NULL;
#endif

/* Init OpenGL */
int CGX_GL_Init(_THIS)
{
#if SDL_VIDEO_OPENGL
   int i = 0;
	struct TagItem attributes [ 14 ]; /* 14 should be more than enough :) */
   struct Window *win = (struct Window *)SDL_Window;

	// default config. Always used...
	attributes[i].ti_Tag = AMA_Window;	attributes[i++].ti_Data = (unsigned long)win;
	attributes[i].ti_Tag = AMA_Left;		attributes[i++].ti_Data = 0;
	attributes[i].ti_Tag = AMA_Bottom;	attributes[i++].ti_Data = 0;
	attributes[i].ti_Tag = AMA_Width;	attributes[i++].ti_Data = win->Width-win->BorderLeft-win->BorderRight;
	attributes[i].ti_Tag = AMA_Height;	attributes[i++].ti_Data = win->Height-win->BorderBottom-win->BorderTop;
	attributes[i].ti_Tag = AMA_DirectRender; attributes[i++].ti_Data = GL_TRUE;

	// double buffer ?
	attributes[i].ti_Tag = AMA_DoubleBuf;
	if ( this->gl_config.double_buffer ) {
		attributes[i++].ti_Data = GL_TRUE;
	}
	else {
		attributes[i++].ti_Data = GL_FALSE;
	}
	// RGB(A) Mode ?
	attributes[i].ti_Tag = AMA_RGBMode;
	if ( this->gl_config.red_size   != 0 &&
	     this->gl_config.blue_size  != 0 &&
	     this->gl_config.green_size != 0 ) {
		attributes[i++].ti_Data = GL_TRUE;
	}
	else {
		attributes[i++].ti_Data = GL_FALSE;
	}
	// no depth buffer ?
	if ( this->gl_config.depth_size == 0 ) {
		attributes[i].ti_Tag = AMA_NoDepth;
		attributes[i++].ti_Data = GL_TRUE;
	}
	// no stencil buffer ?
	if ( this->gl_config.stencil_size == 0 ) {
		attributes[i].ti_Tag = AMA_NoStencil;
		attributes[i++].ti_Data = GL_TRUE;
	}
	// no accum buffer ?
	if ( this->gl_config.accum_red_size   != 0 &&
	     this->gl_config.accum_blue_size  != 0 &&
	     this->gl_config.accum_green_size != 0 ) {
		attributes[i].ti_Tag = AMA_NoAccum;
		attributes[i++].ti_Data = GL_TRUE;
	}
	// done...
	attributes[i].ti_Tag	= TAG_DONE;

	glcont = AmigaMesaCreateContext(attributes);
	if ( glcont == NULL ) {
		SDL_SetError("Couldn't create OpenGL context");
		return(-1);
	}
	this->gl_data->gl_active = 1;
	this->gl_config.driver_loaded = 1;

	return(0);
#else
	SDL_SetError("OpenGL support not configured");
	return(-1);
#endif
}

/* Quit OpenGL */
void CGX_GL_Quit(_THIS)
{
#if SDL_VIDEO_OPENGL
	if ( glcont != NULL ) {
		AmigaMesaDestroyContext(glcont);
		glcont = NULL;
		this->gl_data->gl_active = 0;
		this->gl_config.driver_loaded = 0;
	}
#endif
}

/* Attach context to another window */
int CGX_GL_Update(_THIS)
{
#if SDL_VIDEO_OPENGL
	struct TagItem tags[2];
	struct Window *win = (struct Window*)SDL_Window;
	if(glcont == NULL) {
		return -1; //should never happen
	}
	tags[0].ti_Tag = AMA_Window;
	tags[0].ti_Data = (unsigned long)win;
	tags[1].ti_Tag = TAG_DONE;
	AmigaMesaSetRast(glcont, tags);

	return 0;
#else
	SDL_SetError("OpenGL support not configured");
	return -1;
#endif
}

#if SDL_VIDEO_OPENGL

/* Make the current context active */
int CGX_GL_MakeCurrent(_THIS)
{
	if(glcont == NULL)
		return -1;

	AmigaMesaMakeCurrent(glcont, glcont->buffer);
	return 0;
}

void CGX_GL_SwapBuffers(_THIS)
{
	AmigaMesaSwapBuffers(glcont);
}

int CGX_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value) {
	GLenum mesa_attrib;

	switch(attrib) {
		case SDL_GL_RED_SIZE:
			mesa_attrib = GL_RED_BITS;
			break;
		case SDL_GL_GREEN_SIZE:
			mesa_attrib = GL_GREEN_BITS;
			break;
		case SDL_GL_BLUE_SIZE:
			mesa_attrib = GL_BLUE_BITS;
			break;
		case SDL_GL_ALPHA_SIZE:
			mesa_attrib = GL_ALPHA_BITS;
			break;
		case SDL_GL_DOUBLEBUFFER:
			mesa_attrib = GL_DOUBLEBUFFER;
			break;
		case SDL_GL_DEPTH_SIZE:
			mesa_attrib = GL_DEPTH_BITS;
			break;
		case SDL_GL_STENCIL_SIZE:
			mesa_attrib = GL_STENCIL_BITS;
			break;
		case SDL_GL_ACCUM_RED_SIZE:
			mesa_attrib = GL_ACCUM_RED_BITS;
			break;
		case SDL_GL_ACCUM_GREEN_SIZE:
			mesa_attrib = GL_ACCUM_GREEN_BITS;
			break;
		case SDL_GL_ACCUM_BLUE_SIZE:
			mesa_attrib = GL_ACCUM_BLUE_BITS;
			break;
		case SDL_GL_ACCUM_ALPHA_SIZE:
			mesa_attrib = GL_ACCUM_ALPHA_BITS;
			break;
		default :
			return -1;
	}

	AmigaMesaGetConfig(glcont->visual, mesa_attrib, value);
	return 0;
}

void *CGX_GL_GetProcAddress(_THIS, const char *proc) {
	void *func = NULL;
	func = AmiGetGLProc(proc);
	return func;
}

int CGX_GL_LoadLibrary(_THIS, const char *path) {
	/* Library is always open */
	this->gl_config.driver_loaded = 1;

	return 0;
}

#endif /* SDL_VIDEO_OPENGL */

