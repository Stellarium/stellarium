// ------------
// PathInfo.cpp
// ------------
/**
* @file
* @brief Re: PathInfo
* @author Achim Klein
* @version 1.7
*/


// --------
// Includes
// --------
#include "PathInfo.hpp"

#include <cstring>


// --------------
// Static members
// --------------

/// this buffer is used by any static method
string PathInfo::m_buffer;

/// this character separates two folders
const char PathInfo::separator('/');


// --------
// PathInfo
// --------
/**
 * The standard-constructor.
 */
PathInfo::PathInfo()
{
	// nothing
}


// ---------
// ~PathInfo
// ---------
/**
 * Destructor without any task.
 */
PathInfo::~PathInfo()
{
	// nothing
}


// --------
// getDrive
// --------
/**
 * Returns the drive's letter from the passed path.
 */
const char* PathInfo::getDrive(const char* Path)
{
	m_buffer = "";

	string path(Path);

	int colon = path.find(':');

	if (colon > 0)
	{
		m_buffer = path[colon - 1];
	}

	return m_buffer.c_str();
}


// ------------
// getDirectory
// ------------
/**
 * Returns the directory from the passed path.
 */
const char* PathInfo::getDirectory(const char* Path)
{
	m_buffer = "";

	string path(Path);

	// the highest valid index
	int last = path.size() - 1;

	// no valid index
	if (last < 0) return "";

	// last separator
        int sep = path.rfind(separator);

	// separator is last character
	if (sep == last)
	{
		m_buffer = path;
	}

	// separator is anywhere else
	else if (sep != string::npos)
	{
		m_buffer = path.substr(0, (sep + 1));
	}

	return m_buffer.c_str();
}


// ---------
// getFolder
// ---------
/**
 * Returns the file's folder from the passed path.
 */
const char* PathInfo::getFolder(const char* Path)
{
	m_buffer = "";

	// get directory
	string dir = getDirectory(Path);

	// next to last separator
        unsigned int sep;
	sep = dir.rfind(separator);
	sep = dir.rfind(separator, (sep - 1));

	// no next to last separator
	if (sep == string::npos) return "";

	// get folder
	m_buffer = dir.substr((sep + 1), (dir.size() - sep - 2));

	return m_buffer.c_str();
}


// -------
// getName
// -------
/**
 * Returns the file's name from the passed path.
 */
const char* PathInfo::getName(const char* Path)
{
	m_buffer = "";

	string path(Path);

	// number of characters in path
	int psize = path.size(); 

	// number of characters in dir
	int dsize = strlen(getDirectory(Path));

	if (dsize < psize)
	{
		m_buffer = path.substr(dsize, (psize - dsize));
	}

	// there is no name
	else return "";

	return m_buffer.c_str();
}


// --------
// getTitle
// --------
/**
 * Returns the file's title from the passed path.
 */
const char* PathInfo::getTitle(const char* Path)
{
	m_buffer = "";

	// get name
	string name = getName(Path);

	// last dot
        unsigned int dot = name.rfind(".");

	// no dot
	if (dot == string::npos)
	{
		m_buffer = name;
	}

	// dot is anywhere else
	else
	{
		m_buffer = name.substr(0, dot);
	}

	return m_buffer.c_str();
}


// ------------
// getExtension
// ------------
/**
 * Returns the file's extension from the passed path.
 */
const char* PathInfo::getExtension(const char* Path)
{
	m_buffer = "";

	// get name
	string name = getName(Path);

	// last dot
        unsigned int dot = name.rfind(".");

	// no dot
	if (dot == string::npos) return "";

	// get extension
	m_buffer = name.substr((dot + 1), (name.size() - dot - 1));

	return m_buffer.c_str();
}


// ---------
// getPathAs
// ---------
/**
 * Returns the passed path with this new extension.
 */
const char* PathInfo::getPathAs(const char* Path, const char* Extension)
{
	m_buffer = "";

	string ex = Extension;

	while(ex.size() && (ex[0] == '.'))
	{
		ex.erase(0, 1);
	}

	string dir = getDirectory(Path);
	string title = getTitle(Path);

	// no title -> no extension
	if (title.empty()) return "";

	m_buffer  = dir;
	m_buffer += title;
	m_buffer += ".";
	m_buffer += ex;

	return m_buffer.c_str();
}


// ----------
// getAbsPath
// ----------
/**
 * Returns the absolute path starting at Origin.
 */
const char* PathInfo::getAbsPath(const char* RelPath, const char* Origin)
{
	m_buffer = "";

	string relpath(RelPath);
	string name = getName(RelPath);
	string dir  = getDirectory(Origin);

	m_buffer = "";

	// RelPath is already absolute
	if (relpath.find(':') != string::npos)
	{
		m_buffer = relpath;
		return m_buffer.c_str();
	}

	// separated folders
	vector<string> fPath;
	vector<string> fOrigin;

	// split into folders
	doSplitPath(relpath, fPath);
	doSplitPath(dir, fOrigin);

	// nothing splitted
	if (fPath.empty()) return "";
	if (fOrigin.empty()) return "";

	// remove 'current dir'
	if (fPath[0] == ".") fPath.erase(fPath.begin());
	if (fOrigin[0] == ".") fOrigin.erase(fOrigin.begin());

	// nemove filename
	if (name.size()) fPath.pop_back();

	// number of dotted folders
        unsigned int dotted;

	// count dotted folders
	for(dotted = 0; dotted < fPath.size(); dotted++)
	{
		if (fPath[dotted] != "..") break;
	}

	// add absolute pieces
        for(unsigned int abs = 0; abs < (fOrigin.size() - dotted); abs++)
	{
		m_buffer += fOrigin[abs];
		m_buffer += separator;
	}

	// add relative pieces
        for(unsigned int rel = dotted; rel < fPath.size(); rel++)
	{
		m_buffer += fPath[rel];
		m_buffer += separator;
	}

	// add filename
	m_buffer += name;

	return m_buffer.c_str();
}


// ----------
// getRelPath
// ----------
/**
 * Returns the relative path starting at Origin.
 */
const char* PathInfo::getRelPath(const char* AbsPath, const char* Origin)
{
	m_buffer = "";

	string abspath(AbsPath);
	string name = getName(AbsPath);
	string dir  = getDirectory(Origin);

	m_buffer = "";

	// AbsPath is already relative
	if (abspath.find(':') == string::npos)
	{
		m_buffer = abspath;
		return m_buffer.c_str();
	}

	// separated folders
	vector<string> fPath;
	vector<string> fOrigin;

	// split into folders
	doSplitPath(abspath, fPath);
	doSplitPath(dir, fOrigin);

	// nothing splitted
	if (fPath.empty()) return "";
	if (fOrigin.empty()) return "";

	// remove 'current dir'
	if (fPath[0] == ".") fPath.erase(fPath.begin());
	if (fOrigin[0] == ".") fOrigin.erase(fOrigin.begin());

	// remove filename
	if (name.size()) fPath.pop_back();

	// number of equal folders
        unsigned int equal;

	// count equal folders (from the beginning)
	for(equal = 0; ((equal < fOrigin.size()) && (equal < fPath.size())); equal++)
	{
		// get length
		int size1 = fPath[equal].size();
		int size2 = fOrigin[equal].size();

		// different folders found (length)
		if (size1 != size2)
		{
			break;
		}

		// different folders found (lexicographically)
		else if (strncasecmp(fPath[equal].c_str(), fOrigin[equal].c_str(), size1))
		{
			break;
		}
	}

	// different drives
	if (equal == 0)
	{
		m_buffer = abspath;
		return m_buffer.c_str();
	}

	// move up
        for(unsigned int up = equal; up < fOrigin.size(); up++)
	{
		m_buffer += "..";
		m_buffer += separator;
	}

	// move down
        for(unsigned int dn = equal; dn < fPath.size(); dn++)
	{
		m_buffer += fPath[dn];
		m_buffer += separator;
	}

	// add filename
	m_buffer += name;

	// current directory
	if (m_buffer.empty())
	{
		m_buffer = ".\\";
	}

	return m_buffer.c_str();
}


// -----------
// doSplitPath
// -----------
/**
 * Divides a path up into pieces.
 */
int PathInfo::doSplitPath(const string& Path, vector<string>& Pieces)
{
	string piece;

	// find separators
        for(unsigned int i = 0; i < Path.size(); i++)
	{
		char c = Path[i];

		if (c == separator)
		{
			if (piece.empty()) continue;

			Pieces.push_back(piece);

			piece = "";
		}

		else piece += c;
	}

	// add last piece
	if (piece.size())
	{
		Pieces.push_back(piece);
	}

	// return number of pieces
	return Pieces.size();
}

