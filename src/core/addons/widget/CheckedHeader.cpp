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

#include "CheckedHeader.hpp"

CheckedHeader::CheckedHeader(int section, Qt::Orientation orientation, QWidget *parent)
	: QHeaderView(orientation, parent)
	, m_iSection(section)
	, m_bChecked(false)
{
}

void CheckedHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
	painter->save();
	QHeaderView::paintSection(painter, rect, logicalIndex);
	painter->restore();

	if (logicalIndex == m_iSection)
	{
		QStyleOptionButton option;

		if (isEnabled())
		{
			option.state |= QStyle::State_Enabled;
		}

		QSize size(10,10);
		QPoint point(rect.x()+8, rect.y() + rect.height()/2 - size.height()/2);
		option.rect = QRect(point, size);

		if (m_bChecked)
		{
			option.state |= QStyle::State_On;
		}
		else
		{
			option.state |= QStyle::State_Off;
		}

		//style()->drawControl(QStyle::CE_CheckBox, &option, painter);
		style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
	}
}

void CheckedHeader::mousePressEvent(QMouseEvent *event)
{
	if (!isEnabled() || logicalIndexAt(event->pos()) != m_iSection)
	{
		QHeaderView::mousePressEvent(event);
		return;
	}
	setChecked(!m_bChecked);
}

void CheckedHeader::setChecked(bool checked)
{
	if (!isEnabled() || checked == m_bChecked)
	{
		return;
	}
	m_bChecked = checked;
	updateSection(m_iSection);
	emit(toggled(m_bChecked));
}
