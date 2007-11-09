/*
* ---------------- www.spacesimulator.net --------------
*   ---- Space simulators and 3d engine tutorials ----
*
* Author: Damiano Vitulli <info@spacesimulator.net>
*
* ALL RIGHTS RESERVED
*
*
* Tutorial 4: 3d engine - 3ds models loader
* 
* Include File: texture.cpp
*  
*/



#include <stdio.h>
#include <windows.h>
#include <GL\glut.h>
#include "texture.h"



/**********************************************************
*
* VARIABLES DECLARATION
*
*********************************************************/

int num_texture=-1; //Counter to keep track of the last loaded texture



/**********************************************************
*
* FUNCTION LoadBitmap(char *)
*
* This function loads a bitmap file and return the OpenGL reference ID to use that texture
*
*********************************************************/

int LoadBitmap(char *filename) 
{
  int i, j=0; //Index variables
  FILE *l_file; //File pointer
  unsigned char *l_texture; //The pointer to the memory zone in which we will load the texture

  // windows.h gives us these types to work with the Bitmap files
  BITMAPFILEHEADER fileheader; 
  BITMAPINFOHEADER infoheader;
  RGBTRIPLE rgb;

  num_texture++; // The counter of the current texture is increased

  if ((l_file = fopen(filename, "rb"))==NULL) return (-1); // Open the file for reading

  fread(&fileheader, sizeof(fileheader), 1, l_file); // Read the fileheader

  fseek(l_file, sizeof(fileheader), SEEK_SET); // Jump the fileheader
  fread(&infoheader, sizeof(infoheader), 1, l_file); // and read the infoheader

  // Now we need to allocate the memory for our image (width * height * color deep)
  l_texture = (byte *) malloc(infoheader.biWidth * infoheader.biHeight * 4);
  // And fill it with zeros
  memset(l_texture, 0, infoheader.biWidth * infoheader.biHeight * 4);

  // At this point we can read every pixel of the image
  for (i=0; i < infoheader.biWidth*infoheader.biHeight; i++)
  {            
    // We load an RGB value from the file
    fread(&rgb, sizeof(rgb), 1, l_file); 

    // And store it
    l_texture[j+0] = rgb.rgbtRed; // Red component
    l_texture[j+1] = rgb.rgbtGreen; // Green component
    l_texture[j+2] = rgb.rgbtBlue; // Blue component
    l_texture[j+3] = 255; // Alpha value
    j += 4; // Go to the next position
  }

  fclose(l_file); // Closes the file stream

  glBindTexture(GL_TEXTURE_2D, num_texture); // Bind the ID texture specified by the 2nd parameter

  //// The next commands sets the texture parameters
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // If the u,v coordinates overflow the range 0,1 the image is repeated
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // The magnification function ("linear" produces better results)
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); //The minifying function

  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); // We don't combine the color with the original surface color, use only the texture map.

  //// Finally we define the 2d texture
  glTexImage2D(GL_TEXTURE_2D, 0, 4, infoheader.biWidth, infoheader.biHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, l_texture);

  //// And create 2d mipmaps for the minifying function
  gluBuild2DMipmaps(GL_TEXTURE_2D, 4, infoheader.biWidth, infoheader.biHeight, GL_RGBA, GL_UNSIGNED_BYTE, l_texture);

  free(l_texture); // Free the memory we used to load the texture

  return (num_texture); // Returns the current texture OpenGL ID
}

