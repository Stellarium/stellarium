/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 
#include "StelModuleMgr.h"
#include "stelapp.h"

#if defined (HAVE_GMODULE) && defined (HAVE_GLIB)
 #include <glib.h>
 #include <gmodule.h>
 // the function signature for getStelmodule()
 typedef StelModule* (* getStelmoduleFunc) (void);
 static getStelmoduleFunc getStelModule = NULL;
#endif

StelModuleMgr::StelModuleMgr() : endIter(modules.end())
{ 
}

StelModuleMgr::~StelModuleMgr()
{
}

StelModule* StelModuleMgr::loadExternalModule(const string& moduleID)
{
	string moduleFullPath = "modules/" + moduleID + "/" + moduleID;
#ifdef WIN32
	moduleFullPath += ".dll";
#else
	moduleFullPath += ".so";
#endif
	moduleFullPath = StelApp::getInstance().getFilePath(moduleFullPath);
#if !defined (HAVE_GMODULE) || !defined (HAVE_GLIB)
	cerr << "This version of stellarium was compiled without enabling dynamic loading of modules." << endl;
	cerr << "Module " << moduleID << " will not be loaded." << endl;
	return NULL;
#else
	if (g_module_supported()==FALSE)
	{
		cerr << "Dynamic loading of modules does not work on this plateform." << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		return NULL;
	}
	// Load module
	GModule* md = g_module_open(moduleFullPath.c_str(), G_MODULE_BIND_LAZY);
	if (md==NULL)
	{
		cerr << "Couldn't find the dynamic library: " << moduleFullPath << ": " << g_module_error() << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		return NULL;
	}

	if (g_module_symbol(md, "getStelModule", (gpointer *)&getStelModule)!=TRUE)
	{
		cerr << "Couldn't find the getStelModule() function in the shared library: " << moduleID << ": " << g_module_error() << endl;
		cerr << "Module " << moduleID << " will not be loaded." << endl;
		g_module_close(md);
		return NULL;
	}
	StelModule* sMod = getStelModule();
	cout << "Loaded external module " << moduleID << "." << endl;
	return sMod;
#endif
}
