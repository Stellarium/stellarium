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

#include <cstdlib>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDebug>

#include "StringArray.hpp"

namespace BigStarCatalogExtension {

void StringArray::initFromFile(const QString& file_name) {
  clear();
  QStringList list;
  QFile f(file_name);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    while(!f.atEnd())
    {
      QString s = QString::fromUtf8(f.readLine());
      s.chop(1);
      list << s;
    }
    f.close();
    size = list.size();
  }
  if (size > 0) {
    array = new QString[size];
    if (array == 0) {
      qCritical() << "ERROR: StringArray::initFromFile: out of memory";
      exit(1);
    }
    for (int i=0;i<size;i++) {
      array[i] = list.at(i);
    }
  }
}

} // namespace BigStarCatalogExtension

