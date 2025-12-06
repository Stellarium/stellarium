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

#ifndef SCMPREVIEWPATHITEM_HPP
#define SCMPREVIEWPATHITEM_HPP

#include <QGraphicsPathItem>

//! @class ScmPreviewPathItem
//! Custom QGraphicsPathItem that allows to display a path defined by 3 points.
class ScmPreviewPathItem : public QGraphicsPathItem
{
public:
	ScmPreviewPathItem();

	// public functions
	void setFirstPoint(QPointF point);
	void setMousePoint(QPointF point);
	void setLastPoint(QPointF point);

	void reset();

public slots:

signals:

protected:

private:
	QPainterPath currentPath;
};

#endif // SCMPREVIEWPATHITEM_HPP
