/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
 * Copyright (C) 2016 Marcos Cardinot
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelObjectModule.hpp"

StelObjectModule::StelObjectModule()
 : StelModule()
{
}

StelObjectModule::~StelObjectModule()
{
}

bool StelObjectModule::matchObjectName(const QString& objName, const QString& objPrefix, bool useStartOfWords) const
{
	if (useStartOfWords)
		return objName.startsWith(objPrefix, Qt::CaseInsensitive);
	else
		return objName.contains(objPrefix, Qt::CaseInsensitive);
}

QStringList StelObjectModule::listMatchingObjects(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem <= 0)
		return result;

	QStringList names;
	names << listAllObjects(false) << listAllObjects(true);
	QString fullMatch = "";
	for (const auto& name : qAsConst(names))
	{
		if (!matchObjectName(name, objPrefix, useStartOfWords))
			continue;

		if (name==objPrefix)
			fullMatch = name;
		else
			result.append(name);
		if (result.size() >= maxNbItem)
			break;
	}

	result.sort();
	if (!fullMatch.isEmpty())
		result.prepend(fullMatch);
	return result;
}

QStringList StelObjectModule::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	Q_UNUSED(objType);
	Q_UNUSED(inEnglish);
	return QStringList();
}
