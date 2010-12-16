// ----------
// PathInfo.h
// ----------
/**
* @file
* @brief Re: PathInfo
* @author Achim Klein
* @version 1.3
*/


// -------------------
// One-Definition-Rule
// -------------------
#ifndef _PATHINFO_HPP_
#define _PATHINFO_HPP_


// -------
// Pragmas
// -------
#ifdef WIN32
#pragma warning(disable: 4786)
#endif


// --------
// Includes
// --------
#include <vector>
#include <string>


// ----------
// Namespaces
// ----------
using namespace std;


// --------------------------------
// Definition of the PathInfo class
// --------------------------------
/**
 * This class separates the different sections of a path.
 */
class PathInfo
{

public:

	// ------------
	// Construction
	// ------------

	/// standard-constructor
	PathInfo();

	/// virtual destructor
	virtual ~PathInfo();


	// -----------------
	// Public attributes
	// -----------------

	/// this character separates two folders (may be '/' or '\')
	static const char separator;


	// --------------
	// Static methods
	// --------------

	/// returns the drive's letter from the passed path
	static const char* getDrive(const char* Path);

	/// returns the directory from the passed path
	static const char* getDirectory(const char* Path);

	/// returns the file's folder from the passed path
	static const char* getFolder(const char* Path);

	/// returns the file's name from the passed path
	static const char* getName(const char* Path);

	/// returns the file's title from the passed path
	static const char* getTitle(const char* Path);

	/// returns the file's extension from the passed path
	static const char* getExtension(const char* Path);

	/// returns the passed path with this new extension
	static const char* getPathAs(const char* Path, const char* Extension);

	/// returns the absolute path starting at Origin
	static const char* getAbsPath(const char* RelPath, const char* Origin);

	/// returns the relative path starting at Origin
	static const char* getRelPath(const char* AbsPath, const char* Origin);


protected:

	// ----------------
	// Internal methods
	// ----------------

	/// divides a path up into pieces
	static int doSplitPath(const string& Path, vector<string>& Pieces);


private:

	// ----------
	// Attributes
	// ----------

	/// this buffer is used by any static method
	static string m_buffer;
};


#endif	// #ifndef PATHINFO_H_INCLUDE_NR1
