/*
* Copyright (C) 2003 Fabien Chereau
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef WIN32
#include "glew.h"
#include "shlobj.h"
#endif

#include "stelapp.h"
#include "s_gui.h"

#ifdef HAVE_SDL_MIXER_H
#include "SDL_mixer.h"
#endif

#include "glh/glh_glut.h"

#include <cv.h>
#include "highgui.h"
#include "cxcore.h"

using namespace s_gui;

StelApp::~StelApp()
{
  SDL_FreeCursor(Cursor);
  delete ui;
  delete scripts;
  delete commander;
  delete core;
  if (distorter)
    delete distorter;
  if (pbufglctx)
    wglDeleteContext(pbufglctx);
  if (hpbufdc&&wglReleasePbufferDCARB)
    wglReleasePbufferDCARB(hbuffer, hpbufdc);
  if (hbuffer&&wglDestroyPbufferARB)
    wglDestroyPbufferARB(hbuffer);
  if (VideoWriter)
  {
    cvReleaseVideoWriter(&VideoWriter);
    VideoWriter = NULL;
  }

}

void StelApp::initSDL(int w, int h, int bbpMode, bool fullScreen, string iconFile)
{
  Uint32  Vflags;    // Our Video Flags
  Screen = NULL;

#ifdef HAVE_SDL_MIXER_H

  // Init the SDL library, the VIDEO subsystem    
  // Tony - added timer
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
  {
    // couldn't init audio, so try without
    fprintf(stderr, "Error: unable to open SDL with audio: %s\n", SDL_GetError() );

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
    {
      fprintf(stderr, "Error: unable to open SDL: %s\n", SDL_GetError() );
      exit(-1);
    }
  } 
  else
  {
    /*
    // initialized with audio enabled
    // TODO: only initi audio if config option allows and script needs
    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers))
    {
    printf("Unable to open audio!\n");
    }

    */
  }
#else
  // SDL_mixer is not available - no audio
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER)<0)
  {
    fprintf(stderr, "Unable to open SDL: %s\n", SDL_GetError() );
    exit(-1);
  }
#endif

  // Make sure that SDL_Quit will be called in case of exit()
  atexit(SDL_Quit);

  // Might not work TODO check how to handle that
  //SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24);

  // We want a hardware surface
  Vflags = SDL_HWSURFACE|SDL_OPENGL;//|SDL_DOUBLEBUF;
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,1);
  //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

  // If fullscreen, set the Flag
  if (fullScreen) Vflags|=SDL_FULLSCREEN;

  // Create the SDL screen surface
  if (CreateAviAndExit || CreateAviOnFlyAndExit)
  {
    //In case we use offscreen rendering
    //the window should be small, something like 800*600
    Screen = SDL_SetVideoMode(W_SCREEN, H_SCREEN, bbpMode, Vflags);
  }
  else
  {
    Screen = SDL_SetVideoMode(w, h, bbpMode, Vflags);
  }
  //int dummy;
  //SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &dummy);
  if(!Screen)
  {
    printf("Warning: Couldn't set %dx%d video mode (%s), retrying with stencil size 0\n", w, h, SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
    if (CreateAviAndExit || CreateAviOnFlyAndExit)
    {
      Screen = SDL_SetVideoMode(W_SCREEN, H_SCREEN, bbpMode, Vflags);
    }
    else
    {
      Screen = SDL_SetVideoMode(w, h, bbpMode, Vflags);
    }

    if(!Screen)
    {
      fprintf(stderr, "Error: Couldn't set %dx%d video mode: %s!\n", w, h, SDL_GetError());
      exit(-1);
    }  
  }

  // Disable key repeat
  SDL_EnableKeyRepeat(0, 0);
  SDL_EnableUNICODE(1);

  // set mouse cursor 
  static const char *arrow[] = {
    /* width height num_colors chars_per_pixel */
    "    32    32        3            1",
    /* colors */
    "X c #000000",
    ". c #ffffff",
    "  c None",
    /* pixels */
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "              XXX               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              XXX               ",
    "                                ",
    "                                ",
    "                                ",
    "   XXXXXXXX         XXXXXXXX    ",
    "   X......X         X......X    ",
    "   XXXXXXXX         XXXXXXXX    ",
    "                                ",
    "                                ",
    "                                ",
    "              XXX               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              X.X               ",
    "              XXX               ",
    "                                ",
    "                                ",
    "15,17"
  };

  Cursor = create_cursor(arrow);
  SDL_SetCursor(Cursor);

  // Set the window caption
  SDL_WM_SetCaption(APP_NAME, APP_NAME);

  // Set the window icon
  SDL_Surface *icon = SDL_LoadBMP((iconFile).c_str());
  SDL_WM_SetIcon(icon, NULL);
  SDL_FreeSurface(icon);

  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapBuffers();
  glClear(GL_COLOR_BUFFER_BIT);  

  //The next code directs to use pbuffer instead of the screen
  if (CreateAviAndExit || CreateAviOnFlyAndExit)
  {
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if (wglGetExtensionsStringARB == NULL)
    {
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (wglGetExtensionsStringARB == NULL)", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }
    std::string extensions = wglGetExtensionsStringARB(wglGetCurrentDC());
    if ((extensions.find("WGL_ARB_pixel_format") == std::string::npos)||
      (extensions.find("WGL_ARB_pbuffer") == std::string::npos))
    {
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved.", "Can't produce the video file. (extensions.find(WGL_ARB_pixel_format) == std::string::npos)||(extensions.find(WGL_ARB_pbuffer) == std::string::npos)", 
        MB_ICONERROR); 
      exit(-1);
    }
#define INIT_ENTRY_POINT( funcname, type ) \
  type funcname = (type) wglGetProcAddress(#funcname); \
  if (funcname == NULL) {MessageBox(NULL, "Can't create the pbuffers. The video stream will not be saved.", "Can't produce the video file. (INIT_ENTRY_POINT)", MB_ICONERROR);exit(-1);};
    /* Initialize WGL_ARB_pbuffer entry points. */
    INIT_ENTRY_POINT(  wglCreatePbufferARB, PFNWGLCREATEPBUFFERARBPROC );
    INIT_ENTRY_POINT(  wglGetPbufferDCARB, PFNWGLGETPBUFFERDCARBPROC );
    INIT_ENTRY_POINT(  wglQueryPbufferARB, PFNWGLQUERYPBUFFERARBPROC );
    INIT_ENTRY_POINT(  wglGetPixelFormatAttribivARB,  PFNWGLGETPIXELFORMATATTRIBIVARBPROC );
    INIT_ENTRY_POINT(  wglGetPixelFormatAttribfvARB,  PFNWGLGETPIXELFORMATATTRIBFVARBPROC );
    INIT_ENTRY_POINT(  wglChoosePixelFormatARB,  PFNWGLCHOOSEPIXELFORMATARBPROC );
#define INIT_ENTRY_POINT2( funcname, type ) \
  funcname = (type) wglGetProcAddress(#funcname); \
  if (funcname == NULL) {MessageBox(NULL, "Can't create the pbuffers. The video stream will not be saved.", "Can't produce the video file. (INIT_ENTRY_POINT)", MB_ICONERROR);exit(-1);};
    INIT_ENTRY_POINT2(  wglReleasePbufferDCARB, PFNWGLRELEASEPBUFFERDCARBPROC );
    INIT_ENTRY_POINT2(  wglDestroyPbufferARB, PFNWGLDESTROYPBUFFERARBPROC );
    ScreenDC = wglGetCurrentDC();
    ScreenGLRC = wglGetCurrentContext();

    // Get ready to query for a suitable pixel format that meets our
    // minimum requirements.
    const int MAX_ATTRIBS = 100;
    const int MAX_PFORMATS = 100;
    int iattributes[2*MAX_ATTRIBS];
    float fattributes[2*MAX_ATTRIBS];
    int niattribs = 0;
    // Attribute arrays must be “0” terminated – for simplicity, first
    // just zero-out the array then fill from left to right.
    for ( int a = 0; a < 2*MAX_ATTRIBS; a++ )
    {
      iattributes[a] = 0;
      fattributes[a] = 0.0;
    }
    // Since we are trying to create a pbuffer, the pixel format we
    // request (and subsequently use) must be “p-buffer capable”.
    iattributes[2*niattribs ] = WGL_DRAW_TO_PBUFFER_ARB;
    iattributes[2*niattribs+1] = true;
    niattribs++;
    // We require a minimum of 24-bit depth.
    iattributes[2*niattribs ] = WGL_DEPTH_BITS_ARB;
    iattributes[2*niattribs+1] = 24;
    niattribs++;
    // We require a minimum of 8-bits for each R, G, B, and A.
    iattributes[2*niattribs ] = WGL_RED_BITS_ARB;
    iattributes[2*niattribs+1] = 8;
    niattribs++;
    iattributes[2*niattribs ] = WGL_GREEN_BITS_ARB;
    iattributes[2*niattribs+1] = 8;
    niattribs++;
    iattributes[2*niattribs ] = WGL_BLUE_BITS_ARB;
    iattributes[2*niattribs+1] = 8;
    niattribs++;
    iattributes[2*niattribs ] = WGL_ALPHA_BITS_ARB;
    iattributes[2*niattribs+1] = 8;
    niattribs++;
    iattributes[2*niattribs ] = WGL_STENCIL_BITS_ARB;
    iattributes[2*niattribs+1] = 8;
    niattribs++;
    // Now obtain a list of pixel formats that meet these minimum
    // requirements.
    int pformat[MAX_PFORMATS];
    unsigned int nformats;
    if ( !wglChoosePixelFormatARB( ScreenDC, iattributes, fattributes,
      MAX_PFORMATS, pformat, &nformats ) )
    {
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (wglChoosePixelFormatARB)", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }
    if (nformats <= 0)
    {
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (nformats <= 0)", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }
    //Choose any of available formats
    int format = pformat[0];
    int iattribs[3];
    iattribs[0] = WGL_PBUFFER_LARGEST_ARB;
    iattribs[1] = 1;
    iattribs[2] = 0;
    hbuffer = wglCreatePbufferARB( ScreenDC, format, screenW, screenH, iattribs );
    if (hbuffer == NULL)
    {
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (hbuffer == NULL)", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }
    hpbufdc = wglGetPbufferDCARB( hbuffer );
    if (hpbufdc == NULL)
    {
      wglDestroyPbufferARB(hbuffer);
      hbuffer = NULL;
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (hbuffer == NULL)", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }
    pbufglctx = wglCreateContext(hpbufdc);
    if (pbufglctx == NULL)
    {
      wglReleasePbufferDCARB(hbuffer, hpbufdc);
      hpbufdc = NULL;
      wglDestroyPbufferARB(hbuffer);
      hbuffer = NULL;
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (pbufglctx == NULL)", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }

    if (!wglMakeCurrent(hpbufdc, pbufglctx))
    {
      wglDeleteContext(pbufglctx);
      pbufglctx = NULL;
      wglReleasePbufferDCARB(hbuffer, hpbufdc);
      hpbufdc = NULL;
      wglDestroyPbufferARB(hbuffer);
      hbuffer = NULL;
      MessageBox(NULL, 
        "Can't create the pbuffers. The video stream will not be saved. (!wglMakeCurrent(hpbufdc, pbufglctx))", "Can't produce the video file.", 
        MB_ICONERROR); 
      exit(-1);
    }; 
  }
}

string StelApp::getVideoModeList(void) const
{
  SDL_Rect **modes;
  int i;
  /* Get available fullscreen/hardware modes */
  modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
  /* Check is there are any modes available */
  if(modes == (SDL_Rect **)0)
  {
    return "No modes available!\n";
  }
  /* Check if our resolution is restricted */
  if(modes == (SDL_Rect **)-1)
  {
    return "All resolutions available.\n";
  }
  else
  {
    /* Print valid modes */
    ostringstream modesstr;
    for(i=0;modes[i];++i)
      modesstr << modes[i]->w << "x" << modes[i]->h << endl;
    return modesstr.str();
  }
}

// from an sdl wiki
SDL_Cursor* StelApp::create_cursor(const char *image[])
{
  int i, row, col;
  Uint8 data[4*32];
  Uint8 mask[4*32];
  int hot_x, hot_y;

  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      switch (image[4+row][col]) {
        case 'X':
          data[i] |= 0x01;
          mask[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
      }
    }
  }
  sscanf(image[4+row], "%d,%d", &hot_x, &hot_y);
  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}


// Terminate the application
void StelApp::terminateApplication(void)
{
  static SDL_Event Q;            // Send a SDL_QUIT event
  Q.type = SDL_QUIT;            // To the SDL event queue
  if(SDL_PushEvent(&Q) == -1)        // Try to send the event
  {
    printf("SDL_QUIT event can't be pushed: %s\n", SDL_GetError() );
    exit(-1);
  }
}

enum {
  USER_EVENT_TICK
};


Uint32 mytimer_callback(Uint32 interval, void *param)
{
  SDL_Event event;
  event.type = SDL_USEREVENT;
  event.user.type = SDL_USEREVENT;
  event.user.code = USER_EVENT_TICK;
  event.user.data1 = NULL;
  event.user.data2 = NULL;
  if(SDL_PushEvent(&event) == -1)
  {
    printf("User tick event can't be pushed: %s\n", SDL_GetError() );
    exit(-1);
  }

  // End this timer.
  return 0;
}

void ConvertFromOpenGL2OpenCV(unsigned char* pixels, IplImage* image, int screenWidth, int screenHeight)
{
  // read pixels starting from the end
  for (int HIndex=0; HIndex<screenHeight; HIndex++)
  {
    memcpy(((char*)image->imageData) + image->widthStep * HIndex,
      pixels + 3*screenWidth * (screenHeight-HIndex-1), screenWidth*3);
  }

  // invert RGB triple (R instead of B)
  for (int HIndex = 0; HIndex < screenHeight; HIndex++)
  {
    for (int WIndex = 0; WIndex < screenWidth; WIndex++)
    {
      char Temp = image->imageData[WIndex*3 + 2 + HIndex*image->widthStep];

      image->imageData[WIndex*3 + 2 + HIndex*image->widthStep] = 
        image->imageData[WIndex*3 + HIndex*image->widthStep];

      image->imageData[WIndex*3 + HIndex*image->widthStep] = Temp;
    }
  }
}

void StelApp::start_main_loop()
{
  bool AppVisible = true;      // At The Beginning, Our App Is Visible
  enum S_GUI_VALUE bt;
  Uint32 last_event_time = SDL_GetTicks();
  // How fast the objects on-screen are moving, in pixels/millisecond.
  double animationSpeed = 0;

  // Hold the value of SDL_GetTicks at start of main loop (set 0 time)
  LastCount = SDL_GetTicks();

  VideoWriter = NULL;

  // Purpose is to create VideoWriter.
  if (CreateAviAndExit)
  {
    CvSize FrameSize;
    FrameSize.width = screenW;
    FrameSize.height = screenH;
    double fps = 25.0;

    // Delete video output file
    DeleteFile(VideoOutput.c_str());

    if (VideoCodec.size() >= 4 || VideoCodec == "-1")
    {
      if (VideoCodec == "-1")
      {
        VideoWriter = cvCreateVideoWriter( VideoOutput.c_str(), -1, fps, FrameSize);
      }
      else
      {
        VideoWriter = cvCreateVideoWriter( VideoOutput.c_str(), CV_FOURCC(VideoCodec[0],VideoCodec[1],VideoCodec[2],VideoCodec[3]), fps, FrameSize);
      }
      if (VideoWriter == NULL)
      {
        VideoWriter = cvCreateVideoWriter( VideoOutput.c_str(), -1, fps, FrameSize);
      }
    }
    else
    {
      VideoWriter = cvCreateVideoWriter( VideoOutput.c_str(), -1, fps, FrameSize);
    }

    if (VideoWriter == NULL)
    {
      quit();
    }
  }

  // if we will work with ShowCreator simultaneously, we have to
  // 1. open needed events
  // 2. map file view
  if (CreateAviOnFlyAndExit)
  {
    hReadEvent = 0;
    hWriteEvent = 0;
    hQuitEvent = 0;
    hMapping = 0;
    onFlyPixels = 0; 

    hReadEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, (LPCSTR)"ReadEvent");
    hWriteEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, (LPCSTR)"WriteEvent");
    hQuitEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, (LPCSTR)"QuitEvent");
    if (!hReadEvent || !hWriteEvent || !hQuitEvent) 
    {
      //printf("OpenEvent error: %d\n", GetLastError());
      quit();
    }

    char  MappingName[] = "onFlyPixelsMapping";
    int sizeOfPixels = 3 * screenW * screenH;

    hMapping = CreateFileMappingA(
      INVALID_HANDLE_VALUE,
      NULL,
      PAGE_READWRITE,
      0,
      sizeOfPixels,
      (LPCSTR)MappingName);
    if (!hMapping)
    {
      //cerr << "Create file mapping failed." << endl;
      quit();
    }

    onFlyPixels = (unsigned char *)MapViewOfFile(
      hMapping,
      FILE_MAP_WRITE,
      0,0,
      sizeOfPixels);
  }

  // Start the main loop
  while(1)
  {
    if ((CreateAviOnFlyAndExit||CreateAviAndExit)&&!scripts->is_playing())
    {
      quit();
    }
    if(SDL_PollEvent(&E))  // Fetch The First Event Of The Queue
    {
      if (E.type != SDL_USEREVENT) {
        last_event_time = SDL_GetTicks();
      }

      switch(E.type)    // And Processing It
      {
      case SDL_QUIT:
        return;
        break;

      case SDL_VIDEORESIZE:
        // Recalculate The OpenGL Scene Data For The New Window
        if (E.resize.h && E.resize.w) core->setViewportSize(E.resize.w, E.resize.h);
        break;

      case SDL_ACTIVEEVENT:
        if (E.active.state & SDL_APPACTIVE)
        {
          // Activity level changed (ie. iconified)
          if (E.active.gain) AppVisible = true; // Activity's been gained
          else AppVisible = false;
        }
        break;

      case SDL_MOUSEMOTION:
        handleMove(E.motion.x,E.motion.y);
        break;

      case SDL_MOUSEBUTTONDOWN:
        // Convert the name from GLU to my GUI
        switch (E.button.button)
        {
        case SDL_BUTTON_RIGHT : bt=S_GUI_MOUSE_RIGHT; break;
        case SDL_BUTTON_LEFT :  bt=S_GUI_MOUSE_LEFT; break;
        case SDL_BUTTON_MIDDLE : bt=S_GUI_MOUSE_MIDDLE; break;
        case SDL_BUTTON_WHEELUP : bt=S_GUI_MOUSE_WHEELUP; break;
        case SDL_BUTTON_WHEELDOWN : bt=S_GUI_MOUSE_WHEELDOWN; break;
        default : bt=S_GUI_MOUSE_LEFT;
        }
        handleClick(E.button.x,E.button.y,bt,S_GUI_PRESSED);
        break;

      case SDL_MOUSEBUTTONUP:
        // Convert the name from GLU to my GUI
        switch (E.button.button)
        {
        case SDL_BUTTON_RIGHT : bt=S_GUI_MOUSE_RIGHT; break;
        case SDL_BUTTON_LEFT :  bt=S_GUI_MOUSE_LEFT; break;
        case SDL_BUTTON_MIDDLE : bt=S_GUI_MOUSE_MIDDLE; break;
        case SDL_BUTTON_WHEELUP : bt=S_GUI_MOUSE_WHEELUP; break;
        case SDL_BUTTON_WHEELDOWN : bt=S_GUI_MOUSE_WHEELDOWN; break;
        default : bt=S_GUI_MOUSE_LEFT;
        }
        handleClick(E.button.x,E.button.y,bt,S_GUI_RELEASED);
        break;

      case SDL_KEYDOWN:
        // Send the event to the gui and stop if it has been intercepted
        // use unicode translation, since not keyboard dependent
        // however, for non-printing keys must revert to just keysym... !
        if ((E.key.keysym.unicode && !handleKeys(E.key.keysym.sym,E.key.keysym.mod,E.key.keysym.unicode,S_GUI_PRESSED)) ||
          (!E.key.keysym.unicode && !handleKeys(E.key.keysym.sym,E.key.keysym.mod,E.key.keysym.sym,S_GUI_PRESSED)))
        {

          /* Fumio patch... can't use because ignores unicode values and hence is US keyboard specific.
          - what was the reasoning?
          if ((E.key.keysym.sym >= SDLK_LAST && !core->handle_keys(E.key.keysym.unicode,S_GUI_PRESSED)) ||
          (E.key.keysym.sym < SDLK_LAST && !core->handle_keys(E.key.keysym.sym,S_GUI_PRESSED)))
          */

          if (E.key.keysym.sym==SDLK_F1) SDL_WM_ToggleFullScreen(Screen); // Try fullscreen

          // ctrl-s saves screenshot
          if (E.key.keysym.unicode==0x0013 &&  (Screen->flags & SDL_OPENGL))
          {
            string tempName;
            char c[3];
            FILE *fp;

            SDL_Surface * temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screenW, screenH, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
              0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
              0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
              );
            if (temp == NULL) exit(-1);

            unsigned char * pixels = (unsigned char *) malloc(3 * screenW * screenH);
            if (pixels == NULL)
            {
              SDL_FreeSurface(temp);
              exit(-1);
            }

            glReadPixels(0, 0, screenW, screenH, GL_RGB, GL_UNSIGNED_BYTE, pixels);

            for (int i=0; i<screenH; i++)
            {
              memcpy(((char *) temp->pixels) + temp->pitch * i,
                pixels + 3*screenW * (screenH-i-1), screenW*3);
            }
            free(pixels);

            string shotdir;
#if defined(WIN32)
            char path[MAX_PATH];
            path[MAX_PATH-1] = '\0';
            // Previous version used SHGetFolderPath and made app crash on window 95/98..
            //if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path)))
            LPITEMIDLIST tmp;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &tmp)))
            {
              SHGetPathFromIDList(tmp, path);                      
              shotdir = string(path)+"\\";
            }
            else
            {  
              if(getenv("USERPROFILE")!=NULL)
              {
                //for Win XP etc.
                shotdir = string(getenv("USERPROFILE")) + "\\My Documents\\";
              }
              else
              {
                //for Win 98 etc.
                shotdir = "C:\\My Documents\\";
              }
            }

#else
            shotdir = string(getenv("HOME")) + "/";
#endif
#ifdef MACOSX
            shotdir += "/Desktop/";
#endif
            for(int j=0; j<=100; ++j)
            {
              /***** MSVC Stellarium Build *****/
              //Original == snprintf(c,3,"%d",j);
#ifdef __MSVC__
              _snprintf(c,3,"%d",j);
#else
              snprintf(c,3,"%d",j);
#endif
              /***** MSVC Stellarium Build *****/              
              tempName = shotdir + "stellarium" + c + ".bmp";
              fp = fopen(tempName.c_str(), "r");
              if(fp == NULL)
                break;
              else
                fclose(fp);
            }

            SDL_SaveBMP(temp, tempName.c_str());
            SDL_FreeSurface(temp);
            cout << "Saved screenshot to file : " << tempName << endl;
          } // end of screenshot saving
        }
        // Rescue escape in case of lock : CTRL + ESC forces brutal quit
        if (E.key.keysym.sym==SDLK_ESCAPE && (SDL_GetModState() & KMOD_CTRL)) terminateApplication();
        break;

      case SDL_KEYUP:
        handleKeys(E.key.keysym.sym,E.key.keysym.mod,E.key.keysym.sym,S_GUI_RELEASED);
      }
    }
    else  // No events to poll
    {
      // If the application is not visible
      if(!AppVisible && !CreateAviAndExit && !CreateAviOnFlyAndExit)
      {
        // Leave the CPU alone, don't waste time, simply wait for an event
        SDL_WaitEvent(NULL);
      }
      else
      {
        // Compute how many fps we should run at to get 1 pixel movement each frame.
        double frameRate = 1000. * animationSpeed;
        // If there was user action in the last 2.5 seconds, shoot for the max framerate.
        if (SDL_GetTicks() - last_event_time < 2500 || frameRate > maxfps)
        {
          frameRate = maxfps;
        }
        if (frameRate < minfps)
        {
          frameRate = minfps;
        }

        if (CreateAviAndExit || CreateAviOnFlyAndExit)
        {
          frameRate = 25;
          TickCount = LastCount + 1000/frameRate;
        }
        else
        {
          TickCount = SDL_GetTicks();      // Get present ticks
        }

        // Wait a while if drawing a frame right now would exceed our preferred framerate.
        if (TickCount-LastCount < 1000./frameRate)
        {
          unsigned int delay = (unsigned int) (1000./frameRate) - (TickCount-LastCount);
          //          printf("delay=%d\n", delay);
          if (delay < 15) {
            // Less than 15ms, just do a dumb wait.
            SDL_Delay(delay);
          } else {
            // A longer delay. Use this timer song and dance so
            // that the app is still responsive if the user does something.
            if (!SDL_AddTimer(delay, mytimer_callback, NULL))
            {
              cerr << "Error: couldn't create an SDL timer: " << SDL_GetError() << endl;
            }
            SDL_WaitEvent(NULL);
          }
        }

        if (!CreateAviAndExit && !CreateAviOnFlyAndExit)
        {
          TickCount = SDL_GetTicks();      // Get present ticks
        }
        this->update(TickCount-LastCount);  // And update the motions and data
        double squaredDistance = this->draw(TickCount-LastCount);  // Do the drawings!
        animationSpeed = sqrt(squaredDistance) / (TickCount-LastCount);
        LastCount = TickCount;        // Save the present tick probing
        SDL_GL_SwapBuffers();        // And swap the buffers

        // fields for working with pictiures in OpenCV
        CvSize FrameSize;
        FrameSize.width = screenW;
        FrameSize.height = screenH;

        //Create OpenCV images with offscreen and screen pictures 
        IplImage* FrameImage = cvCreateImage(FrameSize, 8, 3);

        unsigned char* pixels;

        if (CreateAviAndExit)
        {
          pixels = (unsigned char *) malloc(3 * screenW * screenH);
          if (pixels == NULL)
          {
            quit();
          }

          glReadPixels(0, 0, screenW, screenH, GL_RGB, GL_UNSIGNED_BYTE, pixels);

          // We must do the following things on this step:
          //1. Read data from the pbuffer (actually we have them in the variable pixels)
          //2. Resize the image so that if has screen resolution 800*600 (with using CV functions)
          //3. Resizing on the previous step requires some conversion from OpenGL to OpenCV format
          // - a possible way for further optimization
          //4. Showing the resized image on the screen so that user can we what we are wowking on 
          //(this requires some back convertion to OpenGL format)
          //5. Saving the grabbed image to the video stream 

          //First fillin FrameImage (convert from OpenCV format)
          ConvertFromOpenGL2OpenCV(pixels, FrameImage, screenW, screenH);

          free(pixels); 

          //Add a new frame to the video output
          cvWriteFrame(VideoWriter,  FrameImage );
        }
        else if (CreateAviOnFlyAndExit)
        {
          WaitForSingleObject(hWriteEvent, INFINITE);
          glReadPixels(0, 0, screenW, screenH, GL_RGB, GL_UNSIGNED_BYTE, onFlyPixels);
          SetEvent(hReadEvent);
          ConvertFromOpenGL2OpenCV(onFlyPixels, FrameImage, screenW, screenH);
        }
        if (CreateAviOnFlyAndExit || CreateAviAndExit)
        { // kornyakov: we can save time a little if don't show frame on window
          CvSize ScreenSize;
          ScreenSize.width = W_SCREEN;
          ScreenSize.height = H_SCREEN;
          IplImage* ScreenImage = cvCreateImage(ScreenSize, 8, 3);

          //Show the image in the window
          //Resize to the screen resolution
          cvResize(FrameImage, ScreenImage);

          //Convert the screen image from OpenCV to OpenGL
          pixels = (unsigned char *) malloc(3 * H_SCREEN * W_SCREEN);

          for (int HIndex = 0; HIndex < H_SCREEN; HIndex++)
          {
            for (int WIndex =0; WIndex < W_SCREEN ; WIndex++)
            {
              pixels[WIndex*3  +HIndex*ScreenImage->widthStep] = 
                ScreenImage->imageData[WIndex*3+2+(H_SCREEN-HIndex-1)*ScreenImage->widthStep];
              pixels[WIndex*3+1+HIndex*ScreenImage->widthStep] = 
                ScreenImage->imageData[WIndex*3+1+(H_SCREEN-HIndex-1)*ScreenImage->widthStep];
              pixels[WIndex*3+2+HIndex*ScreenImage->widthStep] = 
                ScreenImage->imageData[WIndex*3+0+(H_SCREEN-HIndex-1)*ScreenImage->widthStep];
            }
          }

          //Show the converted image in the screen windows
          wglMakeCurrent(ScreenDC, ScreenGLRC);
          glDrawPixels(W_SCREEN, H_SCREEN, GL_RGB, GL_UNSIGNED_BYTE, pixels);
          wglMakeCurrent(hpbufdc, pbufglctx);

          free(pixels);
          cvReleaseImage(&ScreenImage);
        }
        cvReleaseImage(&FrameImage);
      }
    }
  }
}
