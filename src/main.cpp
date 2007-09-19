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
#include <clocale>

#include "StelApp.hpp"
#include "Translator.hpp"

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

// Main stellarium procedure
int main(int argc, char **argv)
{
	// Used for getting system date formatting
	setlocale(LC_TIME, "");
	
#ifdef HAVE_LIBCURL
	curl_global_init(CURL_GLOBAL_ALL);
#endif

#ifdef USE_SDL
	StelApp* app = new StelAppSdl(argc, argv);
	app->init();
	app->startMainLoop();
	delete app;
#endif
#ifdef USE_QT4
	StelAppQt4::runStellarium(argc, argv);
#endif

#ifdef HAVE_LIBCURL
	curl_global_cleanup();
#endif
	return 0;
}

