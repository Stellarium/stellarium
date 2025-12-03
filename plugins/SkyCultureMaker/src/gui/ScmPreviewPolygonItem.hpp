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

#ifndef SCMPREVIEWPOLYGONITEM_HPP
#define SCMPREVIEWPOLYGONITEM_HPP

#include <QGraphicsPolygonItem>

//! @class ScmPreviewPolygonItem
//! Simple QGraphicsPolygonItem defined by a polygon, startTime and EndTime.
class ScmPreviewPolygonItem : public QGraphicsPolygonItem
{

public:
	ScmPreviewPolygonItem(bool isTemporary);
	ScmPreviewPolygonItem(int startTime, int endTime);
	ScmPreviewPolygonItem(int startTime, int endTime, bool isTemporary);
	ScmPreviewPolygonItem(int startTime, int endTime, const QPolygonF &polygon);

	// public functions
	int getStartTime() const {return startTime;}
	int getEndTime() const {return endTime;}
	bool existsAtPointInTime(int year) const;

public slots:

signals:

protected:

private:
	bool isTemporary;
	int startTime;
	int endTime;

	void initialize();
};

#endif // SCMPREVIEWPOLYGONITEM_HPP
