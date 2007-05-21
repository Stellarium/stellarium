/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include <string>
#include <iostream>
#include <cstdlib>

#include <config.h>

#include "stellarium.h"
#include "StelApp.hpp"
#include "Translator.hpp"

#if defined(MACOSX) && defined(XCODE)
#include "StelConfig.h"
#endif

#ifdef HAVE_LIBCURL
 #include <curl/curl.h>
#endif

#ifdef USE_SDL
 #include "StelAppSdl.hpp"
#else
 #ifdef USE_QT4
  #include "StelAppQt4.hpp"
 #else
	#error You must define either USE_SDL or USE_QT4
 #endif
#endif

using namespace std;

// Print a beautiful console logo !!
void drawIntro(void)
{
    cout << " -------------------------------------------------------" << endl;
    cout << "[ This is "<< APP_NAME << " - http://www.stellarium.org ]" << endl;
    cout << "[ Copyright (C) 2000-2006 Fabien Chereau et al         ]" << endl;
    cout << " -------------------------------------------------------" << endl;
};

// Main stellarium procedure
int main(int argc, char **argv)
{
	// Used for getting system date formatting
	setlocale(LC_TIME, "");
	
	// Print the console logo..
	drawIntro();

#ifdef HAVE_LIBCURL
	curl_global_init(CURL_GLOBAL_ALL);
#endif

	StelApp* app = NULL;
#ifdef USE_SDL
	app = new StelAppSdl(argc, argv);
#endif
#ifdef USE_QT4
	app = new StelAppQt4(argc, argv);
#endif
	app->init();
	app->startMainLoop();

#ifdef HAVE_LIBCURL
	curl_global_cleanup();
#endif

	delete app;
	return 0;
}

