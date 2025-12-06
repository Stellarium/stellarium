/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScmPolygonInfoTreeItem.hpp"

ScmPolygonInfoTreeItem::ScmPolygonInfoTreeItem(int identifier, int startTime, const QString &endTime, int vertexCount)
	: QTreeWidgetItem()
	, id(identifier)
{
	setText(0, QString::number(startTime));
	setText(1, endTime);
	setText(2, QString::number(vertexCount));

	setTextAlignment(0, Qt::AlignCenter);
	setTextAlignment(1, Qt::AlignCenter);
	setTextAlignment(2, Qt::AlignCenter);
}

int ScmPolygonInfoTreeItem::getId() const
{
	return id;
}
