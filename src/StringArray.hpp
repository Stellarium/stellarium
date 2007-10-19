/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
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

#ifndef _STRING_ARRAY_HPP_
#define _STRING_ARRAY_HPP_

#include <string>

using std::string;

namespace BigStarCatalogExtension {

// simple class for retrieving strings from a file: each string is
// a single line. You can access the lines of a file by the line number.

class StringArray {
public:
  StringArray(void) : array(0),size(0) {}
  ~StringArray(void) {clear();}
  void clear(void) {if (array) {delete[] array;array = 0;} size= 0;}
  int getSize(void) const {return size;}
  string operator[](int i) const {return ((0<=i && i<size) ? array[i] : "");}
  void initFromFile(const char *file_name);
private:
  string *array;
  int size;
};

} // namespace BigStarCatalogExtension

#endif
