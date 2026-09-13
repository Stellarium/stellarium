/*
 * Stellarium
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SEPARATORLISTWIDGETITEM_HPP
#define SEPARATORLISTWIDGETITEM_HPP

#include <QListWidgetItem>

//! @class SeperatorListWidgetItem
//! Special Seperator element for a QListWidget.
class SeparatorListWidgetItem : public QListWidgetItem
{
public:
	//! Constructor with region key for reliable matching
	SeparatorListWidgetItem(const QString &text, const QString &regionKey, QListWidget *parent=nullptr);
	
	//! Get the original (untranslated) region key for matching
	QString regionKey() const { return m_regionKey; }

private:
	QString m_regionKey;
};

#endif // SEPARATORLISTWIDGETITEM_HPP
