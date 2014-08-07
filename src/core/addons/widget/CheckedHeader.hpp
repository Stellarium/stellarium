/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#ifndef _CHECKEDHEADER_HPP_
#define _CHECKEDHEADER_HPP_

#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>

class CheckedHeader : public QHeaderView
{
	Q_OBJECT
public:
	explicit CheckedHeader(int section, Qt::Orientation orientation, QWidget *parent = 0);

	void setChecked(bool checked);

signals:
	void toggled(bool checked);

protected:
	void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
	void mousePressEvent(QMouseEvent *event);

private:
	int m_iSection;
	bool m_bChecked;
};

#endif // _CHECKEDHEADER_HPP_
